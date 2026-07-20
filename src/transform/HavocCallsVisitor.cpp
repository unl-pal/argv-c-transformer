// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/HavocCallsVisitor.hpp"

#include "VerifierNames.hpp"

#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <optional>

namespace {

// Returns the VarDecl behind an expression if it's a plain variable
// reference, else null.
const clang::VarDecl *referencedVar(const clang::Expr *E) {
  if (!E)
    return nullptr;
  if (const auto *DRE = clang::dyn_cast<clang::DeclRefExpr>(E->IgnoreParenCasts()))
    return clang::dyn_cast<clang::VarDecl>(DRE->getDecl());
  return nullptr;
}

// Conservative purity check used to decide whether an `if` condition can be
// dropped along with its (now no-op) branches. Anything not explicitly
// recognized here — calls, overloaded operators, volatile accesses, etc. —
// is treated as side-effecting, so we only ever prune conditionals we can
// prove are safe to remove.
//
// `mutableVars` lists variables whose mutation is unobservable (a for-loop's
// init-declared variables, which die with the loop): increments/decrements
// and assignments targeting them are allowed. Everywhere else it's empty.
bool isSideEffectFree(const clang::Expr *E,
                      const std::set<const clang::VarDecl *> &mutableVars = {}) {
  if (!E)
    return true;
  E = E->IgnoreParenCasts();
  switch (E->getStmtClass()) {
  case clang::Stmt::DeclRefExprClass:
  case clang::Stmt::IntegerLiteralClass:
  case clang::Stmt::FloatingLiteralClass:
  case clang::Stmt::CharacterLiteralClass:
  case clang::Stmt::StringLiteralClass:
  case clang::Stmt::GNUNullExprClass:
  case clang::Stmt::UnaryExprOrTypeTraitExprClass: // sizeof / alignof
    return true;
  case clang::Stmt::UnaryOperatorClass: {
    const auto *UO = clang::cast<clang::UnaryOperator>(E);
    if (UO->isIncrementDecrementOp())
      return mutableVars.count(referencedVar(UO->getSubExpr())) != 0;
    return isSideEffectFree(UO->getSubExpr(), mutableVars);
  }
  case clang::Stmt::BinaryOperatorClass: {
    const auto *BO = clang::cast<clang::BinaryOperator>(E);
    if (BO->isAssignmentOp())
      return mutableVars.count(referencedVar(BO->getLHS())) != 0 &&
             isSideEffectFree(BO->getRHS(), mutableVars);
    return isSideEffectFree(BO->getLHS(), mutableVars) &&
           isSideEffectFree(BO->getRHS(), mutableVars);
  }
  case clang::Stmt::ConditionalOperatorClass: {
    const auto *CO = clang::cast<clang::ConditionalOperator>(E);
    return isSideEffectFree(CO->getCond(), mutableVars) &&
           isSideEffectFree(CO->getTrueExpr(), mutableVars) &&
           isSideEffectFree(CO->getFalseExpr(), mutableVars);
  }
  case clang::Stmt::MemberExprClass:
    return isSideEffectFree(clang::cast<clang::MemberExpr>(E)->getBase(), mutableVars);
  case clang::Stmt::ArraySubscriptExprClass: {
    const auto *AS = clang::cast<clang::ArraySubscriptExpr>(E);
    return isSideEffectFree(AS->getBase(), mutableVars) &&
           isSideEffectFree(AS->getIdx(), mutableVars);
  }
  default:
    return false;
  }
}

// Variables declared in a for-loop's init clause are scoped to the loop and
// die with it, so mutating them (e.g. the classic `i++` increment) is not an
// observable side effect.
std::set<const clang::VarDecl *> loopLocalVars(const clang::Stmt *init) {
  std::set<const clang::VarDecl *> vars;
  if (const auto *declStmt = clang::dyn_cast_or_null<clang::DeclStmt>(init)) {
    for (const clang::Decl *D : declStmt->decls()) {
      if (const auto *VD = clang::dyn_cast<clang::VarDecl>(D))
        vars.insert(VD);
    }
  }
  return vars;
}

// A `for` loop's init clause is a statement, not an expression: either a
// bare expression-statement or a declaration (`for (int i = 0; ...)`). A
// loop-scoped declaration with a side-effect-free initializer is itself
// side-effect-free, since the variable it introduces cannot be observed
// outside the loop.
bool isInitSideEffectFree(const clang::Stmt *init) {
  std::set<const clang::VarDecl *> mutableVars = loopLocalVars(init);
  if (!init)
    return true;
  if (const auto *declStmt = clang::dyn_cast<clang::DeclStmt>(init)) {
    for (const auto *D: declStmt->decls()) {
      if (const auto *varDecl = clang::dyn_cast<clang::VarDecl>(D)) {
        if (!isSideEffectFree(varDecl->getInit(), mutableVars))
          return false;
      }
    }
    return true;
  }
  if (const auto *E = clang::dyn_cast<clang::Expr>(init))
    return isSideEffectFree(E);
  return false;
}

} // namespace

HavocCallsVisitor::HavocCallsVisitor(clang::ASTContext *C,
                                     std::shared_ptr<std::set<std::string>> neededSuffixes,
                                     clang::Rewriter &rewriter)
    : _C(C), _NeededSuffixes(neededSuffixes), _Rewriter(rewriter) {};

// Havoc every call to a function from this file so each function body is
// self-contained (intraprocedural): the call's value is replaced by a fresh
// nondet of its return type. Library calls (callee declared in a header,
// e.g. the C standard library) are kept as-is.
bool HavocCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  clang::SourceManager &mgr = _C->getSourceManager();
  clang::SourceLocation loc = E->getExprLoc();
  // Only rewrite calls spelled out in the file being transformed; a macro
  // expansion has no rewritable source range of its own
  if (!mgr.isInMainFile(loc) || loc.isMacroID())
    return true;

  if (const clang::FunctionDecl *callee = E->getDirectCallee()) {
    // Keep nondet calls already injected by the filter step
    if (callee->getIdentifier() && callee->getName().starts_with("__VERIFIER_"))
      return true;
    if (!callee->isImplicit() && !mgr.isInMainFile(callee->getLocation()) &&
        mgr.isInSystemHeader(callee->getLocation()))
      return true;
  }

  clang::QualType returnType = E->getCallReturnType(*_C);
  if (returnType.isNull() || returnType.getTypePtrOrNull() == nullptr)
    return true;
  if (returnType->isVoidType()) {
    // A void call yields no value to havoc; drop it (the statement's
    // semicolon stays behind, leaving an empty statement). Mark it a no-op so
    // an enclosing if-branch made up only of dropped calls can be pruned too.
    _Rewriter.ReplaceText(E->getSourceRange(), "");
    _NoOpStmts.insert(E);
  } else if (std::optional<std::string> suffix = verifierSuffixForType(returnType)) {
    _Rewriter.ReplaceText(E->getSourceRange(), "__VERIFIER_nondet_" + *suffix + "()");
    _NeededSuffixes->emplace(*suffix);
  } else if (returnType->isAnyPointerType() && !returnType->isFunctionPointerType()) {
    // Pointer returns get a havocked-but-valid block (SV-COMP __VERIFIER_nondet_memory).
    // Block size is a fixed guess for now; char pointers are
    // null-terminated so string ops stay in bounds. AddVerifiersConsumer
    // emits the helper definitions when it sees these markers.
    bool isCharPtr = returnType->getPointeeType()->isAnyCharacterType();
    std::string helper = isCharPtr ? "__havoc_cstring" : "__havoc_block";
    // The helpers return char* / void*; cast back to the call's actual
    // return type so e.g. unsigned char* or a struct pointer doesn't end up
    // assigned from an incompatible pointer type.
    _Rewriter.ReplaceText(E->getSourceRange(),
                          "(" + returnType.getAsString() + ")" + helper + "(128)");
    _NeededSuffixes->emplace(helper);
  }
  // Aggregate returns (structs, unions) have no expression-position nondet
  // equivalent; those calls are left as-is
  return true;
}

bool HavocCallsVisitor::isNoOp(const clang::Stmt *S) const {
  if (!S || clang::isa<clang::NullStmt>(S))
    return true;
  return _NoOpStmts.count(S) != 0;
}

bool HavocCallsVisitor::VisitCompoundStmt(clang::CompoundStmt *S) {
  for (const clang::Stmt *child : S->body()) {
    if (!isNoOp(child))
      return true;
  }
  // Empty blocks fall through the loop above and are no-ops too.
  _NoOpStmts.insert(S);
  return true;
}

// Shared prune rule for if/while/do/for: if every branch/body is already a
// no-op and every controlling expression (condition, plus a for-loop's init
// and increment) is side-effect-free, erase the whole statement and mark it
// a no-op so the pruning can propagate to an enclosing statement. `init` and
// `inc` are unused (default null, trivially side-effect-free) outside
// VisitForStmt.
//
// For `if`, this can never change whether the program terminates — dropping
// a dead branch doesn't create or remove divergence. For loops it can: an
// empty body spinning on a side-effect-free condition (`while (n > 0);`) may
// hang, and pruning it turns that hang into termination. That's intentional
// here — such loops are havoc artifacts, not meaningful termination-
// benchmark content. A condition/increment with a real side effect (e.g.
// `while (x-- > 0);`) is kept regardless, since `x` may be observed after
// the loop — except mutations of variables declared in the for-loop's own
// init clause (`for (int i = 0; i < n; i++)`), which die with the loop and
// are therefore unobservable.
bool HavocCallsVisitor::pruneIfNoOp(clang::Stmt *S, clang::SourceLocation keyLoc,
                                    std::initializer_list<const clang::Stmt *> branches,
                                    const clang::Expr *cond, const clang::Stmt *init,
                                    const clang::Expr *inc) {
  clang::SourceManager &mgr = _C->getSourceManager();
  if (!mgr.isInMainFile(keyLoc) || keyLoc.isMacroID())
    return false;
  for (const clang::Stmt *branch : branches) {
    if (!isNoOp(branch))
      return false;
  }
  std::set<const clang::VarDecl *> mutableVars = loopLocalVars(init);
  if (!isSideEffectFree(cond, mutableVars) || !isInitSideEffectFree(init) ||
      !isSideEffectFree(inc, mutableVars))
    return false;
  _Rewriter.ReplaceText(S->getSourceRange(), "");
  _NoOpStmts.insert(S);
  return true;
}

bool HavocCallsVisitor::VisitIfStmt(clang::IfStmt *S) {
  pruneIfNoOp(S, S->getIfLoc(), {S->getThen(), S->getElse()}, S->getCond());
  return true;
}

bool HavocCallsVisitor::VisitWhileStmt(clang::WhileStmt *S) {
  pruneIfNoOp(S, S->getWhileLoc(), {S->getBody()}, S->getCond());
  return true;
}

bool HavocCallsVisitor::VisitDoStmt(clang::DoStmt *S) {
  pruneIfNoOp(S, S->getDoLoc(), {S->getBody()}, S->getCond());
  return true;
}

bool HavocCallsVisitor::VisitForStmt(clang::ForStmt *S) {
  pruneIfNoOp(S, S->getForLoc(), {S->getBody()}, S->getCond(), S->getInit(), S->getInc());
  return true;
}

bool HavocCallsVisitor::shouldTraversePostOrder() { return true; }
