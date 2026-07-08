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

// Conservative purity check used to decide whether an `if` condition can be
// dropped along with its (now no-op) branches. Anything not explicitly
// recognized here — calls, overloaded operators, volatile accesses, etc. —
// is treated as side-effecting, so we only ever prune conditionals we can
// prove are safe to remove.
bool isSideEffectFree(const clang::Expr *E) {
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
      return false;
    return isSideEffectFree(UO->getSubExpr());
  }
  case clang::Stmt::BinaryOperatorClass: {
    const auto *BO = clang::cast<clang::BinaryOperator>(E);
    if (BO->isAssignmentOp())
      return false;
    return isSideEffectFree(BO->getLHS()) && isSideEffectFree(BO->getRHS());
  }
  case clang::Stmt::ConditionalOperatorClass: {
    const auto *CO = clang::cast<clang::ConditionalOperator>(E);
    return isSideEffectFree(CO->getCond()) && isSideEffectFree(CO->getTrueExpr()) &&
           isSideEffectFree(CO->getFalseExpr());
  }
  case clang::Stmt::MemberExprClass:
    return isSideEffectFree(clang::cast<clang::MemberExpr>(E)->getBase());
  case clang::Stmt::ArraySubscriptExprClass: {
    const auto *AS = clang::cast<clang::ArraySubscriptExpr>(E);
    return isSideEffectFree(AS->getBase()) && isSideEffectFree(AS->getIdx());
  }
  default:
    return false;
  }
}

// A `for` loop's init clause is a statement, not an expression: either a
// bare expression-statement or a declaration (`for (int i = 0; ...)`). A
// loop-scoped declaration with a side-effect-free initializer is itself
// side-effect-free, since the variable it introduces cannot be observed
// outside the loop.
bool isInitSideEffectFree(const clang::Stmt *init) {
  if (!init)
    return true;
  if (const auto *declStmt = clang::dyn_cast<clang::DeclStmt>(init)) {
    if (!declStmt->isSingleDecl())
      return false;
    const auto *VD = clang::dyn_cast<clang::VarDecl>(declStmt->getSingleDecl());
    return VD && isSideEffectFree(VD->getInit());
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

bool HavocCallsVisitor::VisitTranslationUnit(clang::TranslationUnitDecl *D) {
  return clang::RecursiveASTVisitor<HavocCallsVisitor>::TraverseDecl(D);
}

bool HavocCallsVisitor::VisitDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitDecl(D);
}

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
    return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);

  if (const clang::FunctionDecl *callee = E->getDirectCallee()) {
    // Keep nondet calls already injected by the filter step
    if (callee->getIdentifier() && callee->getName().starts_with("__VERIFIER_"))
      return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);
    if (!callee->isImplicit() && !mgr.isInMainFile(callee->getLocation()) &&
        mgr.isInSystemHeader(callee->getLocation()))
      return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);
  }

  clang::QualType returnType = E->getCallReturnType(*_C);
  if (returnType.isNull() || returnType.getTypePtrOrNull() == nullptr)
    return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);
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
  return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);
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
// the loop.
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
  if (!isSideEffectFree(cond) || !isInitSideEffectFree(init) || !isSideEffectFree(inc))
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
