// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/HavocCallsVisitor.hpp"

#include "DebugLog.hpp"
#include "VerifierNames.hpp"

#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <optional>

namespace {

std::string locString(clang::SourceManager &mgr, clang::SourceLocation loc) {
  clang::PresumedLoc presumed = mgr.getPresumedLoc(loc);
  if (!presumed.isValid())
    return "<unknown>";
  return std::string(presumed.getFilename()) + ":" + std::to_string(presumed.getLine());
}

const clang::VarDecl *referencedVar(const clang::Expr *E) {
  if (!E)
    return nullptr;
  if (const auto *DRE = clang::dyn_cast<clang::DeclRefExpr>(E->IgnoreParenCasts()))
    return clang::dyn_cast<clang::VarDecl>(DRE->getDecl());
  return nullptr;
}

// Conservative: anything not explicitly recognized here is treated as
// side-effecting, so pruning only ever removes what is provably safe.
//
// `mutableVars` holds variables whose mutation is unobservable (a for-loop's
// init-declared variables, which die with the loop); increments and
// assignments targeting them pass.
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

// A `for` init clause is a statement, not an expression: either a bare
// expression-statement or a declaration. A loop-scoped declaration with a
// side-effect-free initializer is itself side-effect-free.
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

HavocCallsVisitor::HavocCallsVisitor(clang::ASTContext *C, clang::Rewriter &rewriter)
    : _C(C), _Rewriter(rewriter) {};

bool HavocCallsVisitor::TraverseFunctionDecl(clang::FunctionDecl *D) {
  clang::SourceLocation savedHoistPoint = _HoistPoint;
  if (D->isThisDeclarationADefinition()) {
    if (const auto *body = clang::dyn_cast_or_null<clang::CompoundStmt>(D->getBody()))
      _HoistPoint = body->getLBracLoc().getLocWithOffset(1);
  }
  bool result = RecursiveASTVisitor::TraverseFunctionDecl(D);
  _HoistPoint = savedHoistPoint;
  return result;
}

bool HavocCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  clang::SourceManager &mgr = _C->getSourceManager();
  clang::SourceLocation loc = E->getExprLoc();
  // A macro expansion has no rewritable source range of its own.
  if (!mgr.isInMainFile(loc) || loc.isMacroID())
    return true;

  if (const clang::FunctionDecl *callee = E->getDirectCallee()) {
    // Nondet calls already injected upstream.
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
    // Dropping the call leaves the statement's semicolon behind as an empty
    // statement; marking it no-op lets an enclosing branch prune too.
    debugLog(3, "[transform] " + locString(mgr, loc) + ": dropped void call");
    _Rewriter.ReplaceText(E->getSourceRange(), "");
    _NoOpStmts.insert(E);
  } else if (std::optional<std::string> suffix = verifierSuffixForType(returnType)) {
    debugLog(3, "[transform] " + locString(mgr, loc) + ": havocked call -> __VERIFIER_nondet_" +
                    *suffix + "()");
    _Rewriter.ReplaceText(E->getSourceRange(), "__VERIFIER_nondet_" + *suffix + "()");
  } else if (returnType->isAnyPointerType() && !returnType->isFunctionPointerType() &&
             _HoistPoint.isValid()) {
    bool isCharPtr = returnType->getPointeeType()->isAnyCharacterType();
    std::string name = "__havoc_buf" + std::to_string(_HavocCounter++);
    std::string elemType = isCharPtr ? "char" : "unsigned char";
    _Rewriter.InsertTextAfter(_HoistPoint,
                              "\n  " + elemType + " " + name + "[__HAVOC_BLOCK_MAX];");
    std::string call = isCharPtr
                            ? "__havoc_cstring_fill(" + name + ", __HAVOC_BLOCK_MAX)"
                            : "(__VERIFIER_nondet_memory(" + name + ", __HAVOC_BLOCK_MAX), " +
                                  name + ")";
    debugLog(3, "[transform] " + locString(mgr, loc) + ": havocked pointer call -> " + call);
    // Cast back to the call's return type; the buffer's element type differs.
    _Rewriter.ReplaceText(E->getSourceRange(), "(" + returnType.getAsString() + ")" + call);
  }
  // Aggregate returns have no expression-position nondet equivalent; left as-is.
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

// Pruning a loop can change termination: an empty body spinning on a
// side-effect-free condition (`while (n > 0);`) may hang, and pruning turns
// that hang into termination. Intentional - such loops are havoc artifacts,
// not termination-benchmark content.
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
  debugLog(3, "[transform] " + locString(mgr, keyLoc) + ": pruned no-op statement");
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
