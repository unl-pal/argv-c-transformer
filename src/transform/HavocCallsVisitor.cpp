// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/HavocCallsVisitor.hpp"

#include "DebugLog.hpp"
#include "HavocPolicy.hpp"
#include "VerifierNames.hpp"

#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <optional>

namespace {

struct HavocAction {
  enum class Mode { Erase, Inline, Pointer } mode;
  std::string replacement; // Inline only.
  PointerPlan plan;        // Pointer only.
};

// Decides whether a call is havocked, and into what; nullopt leaves it alone.
// Stateless, so callable repeatedly for the same call.
std::optional<HavocAction> classifyCall(const clang::CallExpr *E, clang::ASTContext &C) {
  clang::SourceManager &mgr = C.getSourceManager();
  clang::SourceLocation loc = E->getExprLoc();
  if (!mgr.isInMainFile(loc) || loc.isMacroID()) // a macro expansion has no rewritable range
    return std::nullopt;

  if (const clang::FunctionDecl *callee = E->getDirectCallee()) {
    if (callee->getIdentifier() && callee->getName().starts_with("__VERIFIER_"))
      return std::nullopt;
    if (!callee->isImplicit() && !mgr.isInMainFile(callee->getLocation()) &&
        mgr.isInSystemHeader(callee->getLocation()))
      return std::nullopt;
  }

  clang::QualType returnType = E->getCallReturnType(C);
  if (returnType.isNull() || returnType.getTypePtrOrNull() == nullptr)
    return std::nullopt;

  if (returnType->isVoidType())
    return HavocAction{HavocAction::Mode::Erase, "", {}};

  if (std::optional<std::string> suffix = verifierSuffixForType(returnType))
    return HavocAction{HavocAction::Mode::Inline, "__VERIFIER_nondet_" + *suffix + "()", {}};

  if (returnType->isAnyPointerType()) {
    PointerPlan plan = planPointer(returnType, mgr); // storage/placement are the caller's job
    if (!plan.viable)
      return std::nullopt;
    return HavocAction{HavocAction::Mode::Pointer, "", plan};
  }

  return std::nullopt; // aggregate return: no expression-position nondet equivalent
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

const clang::VarDecl *referencedVar(const clang::Expr *E) {
  if (!E)
    return nullptr;
  if (const auto *DRE = clang::dyn_cast<clang::DeclRefExpr>(E->IgnoreParenCasts()))
    return clang::dyn_cast<clang::VarDecl>(DRE->getDecl());
  return nullptr;
}

// Consumes a `;` immediately following `S`, if there is one - left behind by
// erasing a dropped call or a pruned if/while/do/for with no trailing `;` of its own.
void eatTrailingSemicolon(clang::ASTContext *C, clang::Rewriter &rewriter, const clang::Stmt *S) {
  std::optional<clang::Token> next =
      clang::Lexer::findNextToken(S->getEndLoc(), C->getSourceManager(), C->getLangOpts());
  if (next && next->is(clang::tok::semi))
    rewriter.RemoveText(next->getLocation(), next->getLength());
}

std::string locString(clang::SourceManager &mgr, clang::SourceLocation loc) {
  clang::PresumedLoc presumed = mgr.getPresumedLoc(loc);
  if (!presumed.isValid())
    return "<unknown>";
  return std::string(presumed.getFilename()) + ":" + std::to_string(presumed.getLine());
}

} // namespace

// Anything not explicitly recognized is treated as side-effecting.
// `mutableVars`: a for-loop's own init-declared variables.
bool HavocCallsVisitor::isSideEffectFree(
    const clang::Expr *E, const std::set<const clang::VarDecl *> &mutableVars) const {
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
  case clang::Stmt::CallExprClass: {
    const auto *CE = clang::cast<clang::CallExpr>(E);
    std::optional<HavocAction> action = classifyCall(CE, *_C);
    if (!action)
      return false;
    if (action->mode == HavocAction::Mode::Pointer) {
      bool discarded = false;
      hoistAnchor(CE, discarded); // pure only if the hoisted storage goes unused
      return discarded;
    }
    return true;
  }
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
    return isSideEffectFree(AS->getBase(), mutableVars) && isSideEffectFree(AS->getIdx(), mutableVars);
  }
  default:
    return false;
  }
}

bool HavocCallsVisitor::containsHavocedCall(const clang::Stmt *S) const {
  if (!S)
    return false;
  if (const auto *CE = clang::dyn_cast<clang::CallExpr>(S))
    if (classifyCall(CE, *_C))
      return true;
  for (const clang::Stmt *child : S->children()) {
    if (containsHavocedCall(child))
      return true;
  }
  return false;
}

// init is a declaration or a bare expression-statement, or null if omitted.
bool HavocCallsVisitor::isInitSideEffectFree(
    const clang::Stmt *init, const std::set<const clang::VarDecl *> &mutableVars) const {
  for (const clang::VarDecl *varDecl : mutableVars) {
    if (!isSideEffectFree(varDecl->getInit(), mutableVars))
      return false;
  }
  if (const auto *E = clang::dyn_cast_or_null<clang::Expr>(init))
    return isSideEffectFree(E, mutableVars);
  return true;
}

HavocCallsVisitor::HavocCallsVisitor(clang::ASTContext *C,
                                     std::shared_ptr<std::set<std::string>> neededFwdDecls,
                                     clang::Rewriter &rewriter)
    : _C(C), _NeededFwdDecls(neededFwdDecls), _Rewriter(rewriter) {};

bool HavocCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  std::optional<HavocAction> action = classifyCall(E, *_C);
  if (!action)
    return true;

  std::string where = locString(_C->getSourceManager(), E->getExprLoc());
  if (action->mode == HavocAction::Mode::Erase) {
    debugLog(4, "[transform] " + where + ": dropped void call");
    eraseStmt(E);
    return true;
  }
  if (action->mode == HavocAction::Mode::Pointer)
    return havocPointerReturn(E, action->plan, where);

  debugLog(4, "[transform] " + where + ": havocked call -> " + action->replacement);
  _Rewriter.ReplaceText(E->getSourceRange(), action->replacement);
  return true;
}

const clang::Stmt *HavocCallsVisitor::hoistAnchor(const clang::CallExpr *E, bool &discarded) const {
  discarded = false;
  clang::DynTypedNode node = clang::DynTypedNode::create(*E);
  while (true) {
    clang::DynTypedNodeList parents = _C->getParents(node);
    if (parents.empty())
      return nullptr;
    const clang::DynTypedNode &parent = parents[0];
    if (parent.get<clang::CompoundStmt>()) {
      const clang::Stmt *anchor = node.get<clang::Stmt>();
      if (!anchor)
        return nullptr;
      if (const auto *asExpr = clang::dyn_cast<clang::Expr>(anchor))
        discarded = asExpr->IgnoreParenImpCasts() == E;
      return anchor;
    }
    node = parent;
  }
}

bool HavocCallsVisitor::havocPointerReturn(clang::CallExpr *E, const PointerPlan &plan,
                                           const std::string &where) {
  bool discarded = false;
  const clang::Stmt *anchor = hoistAnchor(E, discarded);
  if (discarded) {
    debugLog(4, "[transform] " + where + ": dropped discarded pointer call");
    eraseStmt(E);
    return true;
  }
  if (!anchor) {
    debugLog(2, "[transform] " + where + ": pointer call has no statement to hoist above; left as-is");
    return true;
  }

  clang::QualType returnType = E->getCallReturnType(*_C);
  std::string stub = "__hret" + std::to_string(_StubCounter++);
  PointerStorage store = renderPointerStorage(plan, returnType, stub, returnType.getAsString(),
                                              /*indent=*/"");

  _Rewriter.InsertText(anchor->getBeginLoc(), store.decls, /*InsertAfter=*/false,
                       /*indentNewLines=*/true);
  _Rewriter.ReplaceText(E->getSourceRange(), store.arg);

  if (!plan.fwdDecl.empty())
    _NeededFwdDecls->insert(plan.fwdDecl);
  debugLog(4, "[transform] " + where + ": havocked pointer call -> stack " + stub);
  return true;
}

// Idempotent: re-removing an already-erased range confuses the Rewriter's delta bookkeeping.
void HavocCallsVisitor::eraseStmt(const clang::Stmt *S) {
  if (!_ErasedStmts.insert(S).second)
    return;
  _Rewriter.ReplaceText(S->getSourceRange(), "");
}

bool HavocCallsVisitor::isNoOp(const clang::Stmt *S) const {
  if (!S || clang::isa<clang::NullStmt>(S))
    return true;
  auto cached = _NoOpMemo.find(S);
  if (cached != _NoOpMemo.end())
    return cached->second;
  return _NoOpMemo.emplace(S, computeNoOp(S)).first->second; // an enclosing statement re-asks about its children
}

bool HavocCallsVisitor::computeNoOp(const clang::Stmt *S) const {
  clang::SourceLocation begin = S->getBeginLoc();
  if (begin.isMacroID() || !_C->getSourceManager().isInMainFile(begin))
    return false; // unrewritable, so never vacuous

  // pure AND contains our own rewrite - never an author's own dead code
  if (const auto *E = clang::dyn_cast<clang::Expr>(S))
    return containsHavocedCall(E) && isSideEffectFree(E, {});

  if (const auto *CS = clang::dyn_cast<clang::CompoundStmt>(S)) {
    for (const clang::Stmt *child : CS->body()) {
      if (!isNoOp(child))
        return false;
    }
    return true;
  }

  if (const auto *ifS = clang::dyn_cast<clang::IfStmt>(S))
    return isNoOp(ifS->getThen()) && isNoOp(ifS->getElse()) &&
           isSideEffectFree(ifS->getCond(), {});

  // pruning may turn a hang into termination - accepted, these are havoc artifacts
  if (const auto *whileS = clang::dyn_cast<clang::WhileStmt>(S))
    return isNoOp(whileS->getBody()) && isSideEffectFree(whileS->getCond(), {});

  if (const auto *doS = clang::dyn_cast<clang::DoStmt>(S))
    return isNoOp(doS->getBody()) && isSideEffectFree(doS->getCond(), {});

  if (const auto *forS = clang::dyn_cast<clang::ForStmt>(S)) {
    std::set<const clang::VarDecl *> mutableVars = loopLocalVars(forS->getInit());
    return isNoOp(forS->getBody()) && isSideEffectFree(forS->getCond(), mutableVars) &&
           isInitSideEffectFree(forS->getInit(), mutableVars) &&
           isSideEffectFree(forS->getInc(), mutableVars);
  }

  return false;
}

bool HavocCallsVisitor::VisitCompoundStmt(clang::CompoundStmt *S) {
  for (const clang::Stmt *child : S->body()) {
    if (!isNoOp(child))
      continue;
    if (clang::isa<clang::NullStmt>(child) || clang::isa<clang::CompoundStmt>(child)) // nothing further to erase
      continue;
    eraseStmt(child);
    eatTrailingSemicolon(_C, _Rewriter, child);
  }
  return true;
}

void HavocCallsVisitor::pruneIfNoOp(clang::Stmt *S, clang::SourceLocation keyLoc) {
  if (!isNoOp(S))
    return;
  debugLog(3, "[transform] " + locString(_C->getSourceManager(), keyLoc) +
                  ": pruned no-op statement");
  eraseStmt(S);
}

bool HavocCallsVisitor::VisitIfStmt(clang::IfStmt *S) {
  pruneIfNoOp(S, S->getIfLoc());
  return true;
}

bool HavocCallsVisitor::VisitWhileStmt(clang::WhileStmt *S) {
  pruneIfNoOp(S, S->getWhileLoc());
  return true;
}

bool HavocCallsVisitor::VisitDoStmt(clang::DoStmt *S) {
  pruneIfNoOp(S, S->getDoLoc());
  return true;
}

bool HavocCallsVisitor::VisitForStmt(clang::ForStmt *S) {
  pruneIfNoOp(S, S->getForLoc());
  return true;
}

bool HavocCallsVisitor::shouldTraversePostOrder() { return true; }
