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

// What a call becomes when havocked. One of three modes:
//   Erase   - void return: drop the call.
//   Inline  - primitive return: replace with a __VERIFIER_nondet_* expression.
//   Pointer - pointer return: hoist stack storage and substitute it (the caller
//             renders and places it; a pointer cannot be a bare expression).
struct HavocAction {
  enum class Mode { Erase, Inline, Pointer } mode;
  std::string replacement; // Inline only.
  PointerPlan plan;        // Pointer only.
};

// Decides whether a call is havocked, and into what. Returns nullopt for every
// call left alone: macro expansions, verifier calls the filter already
// injected, system-header calls, aggregate returns, and pointer returns whose
// plan is not viable.
//
// This is a pure function of the call and its type - it consults no traversal
// state - which is what lets a later pass ask the same question again instead
// of remembering the answer. It is the single source of truth for "do we
// rewrite this call", so the rewrite and the purity check cannot drift apart.
std::optional<HavocAction> classifyCall(const clang::CallExpr *E, clang::ASTContext &C) {
  clang::SourceManager &mgr = C.getSourceManager();
  clang::SourceLocation loc = E->getExprLoc();
  // Only rewrite calls spelled out in the file being transformed; a macro
  // expansion has no rewritable source range of its own
  if (!mgr.isInMainFile(loc) || loc.isMacroID())
    return std::nullopt;

  if (const clang::FunctionDecl *callee = E->getDirectCallee()) {
    // Keep nondet calls already injected by the filter step
    if (callee->getIdentifier() && callee->getName().starts_with("__VERIFIER_"))
      return std::nullopt;
    if (!callee->isImplicit() && !mgr.isInMainFile(callee->getLocation()) &&
        mgr.isInSystemHeader(callee->getLocation()))
      return std::nullopt;
  }

  clang::QualType returnType = E->getCallReturnType(C);
  if (returnType.isNull() || returnType.getTypePtrOrNull() == nullptr)
    return std::nullopt;

  // A void call yields no value to havoc, so it is erased rather than replaced.
  if (returnType->isVoidType())
    return HavocAction{HavocAction::Mode::Erase, "", {}};

  if (std::optional<std::string> suffix = verifierSuffixForType(returnType))
    return HavocAction{HavocAction::Mode::Inline, "__VERIFIER_nondet_" + *suffix + "()", {}};

  if (returnType->isAnyPointerType()) {
    // Pointer returns get a havocked-but-valid block (SV-COMP
    // __VERIFIER_nondet_memory over stack storage). planPointer decides the
    // shape and size from the pointee rather than guessing; the storage and its
    // placement are the caller's job, since a pointer cannot be a bare
    // expression the way a primitive nondet can.
    //
    // A non-viable plan (function pointer, or a record whose fields the callee
    // could not legally dereference after a bulk havoc) leaves the call alone.
    PointerPlan plan = planPointer(returnType, mgr);
    if (!plan.viable)
      return std::nullopt;
    return HavocAction{HavocAction::Mode::Pointer, "", plan};
  }

  // Aggregate returns (structs, unions) have no expression-position nondet
  // equivalent; those calls are left as-is
  return std::nullopt;
}

// Returns the set of loop-local variable declarations.
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

// Returns the VarDecl behind an expression if it's a plain variable reference, else null.
const clang::VarDecl *referencedVar(const clang::Expr *E) {
  if (!E)
    return nullptr;
  if (const auto *DRE = clang::dyn_cast<clang::DeclRefExpr>(E->IgnoreParenCasts()))
    return clang::dyn_cast<clang::VarDecl>(DRE->getDecl());
  return nullptr;
}

// Consumes a `;` immediately following `S`, if there is one. Used to clean up
// a no-op statement that's a direct child of a block: erasing just the
// statement's own text (a dropped call, or a fully-pruned if/while/do/for
// whose own grammar has no trailing `;`) can still leave that `;` behind, so
// this is applied once the statement is known to sit directly in a `{ ... }`
// list, where nothing needs to remain in its place.
void eatTrailingSemicolon(clang::ASTContext *C, clang::Rewriter &rewriter, const clang::Stmt *S) {
  std::optional<clang::Token> next =
      clang::Lexer::findNextToken(S->getEndLoc(), C->getSourceManager(), C->getLangOpts());
  if (next && next->is(clang::tok::semi))
    rewriter.RemoveText(next->getLocation(), next->getLength());
}

// Formats a source location as "file:line" for per-decision debug logging.
std::string locString(clang::SourceManager &mgr, clang::SourceLocation loc) {
  clang::PresumedLoc presumed = mgr.getPresumedLoc(loc);
  if (!presumed.isValid())
    return "<unknown>";
  return std::string(presumed.getFilename()) + ":" + std::to_string(presumed.getLine());
}

} // namespace

// Conservative purity check to decide if an expression - an `if` condition
// dropped with its no-op branches, or a hollowed-out expression statement -
// can be deleted. Anything not explicitly recognized is treated as
// side-effecting. `mutableVars` lists variables whose mutation is allowed
// (a for-loop's init-declared variables, which die with the loop).
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
    // A havocked call is pure by construction: the transform is
    // intraprocedural, so the callee's writes to globals and out-parameters
    // are already discarded, leaving the return value as its only
    // contribution. Deleting it where that value is unused therefore changes
    // nothing. A call we do *not* havoc (system call, aggregate return,
    // non-viable pointer plan) is a real call and stays side-effecting.
    //
    // A pointer return is the exception: havocking it hoists stack storage
    // above the enclosing statement, so that statement must survive - it is
    // pure only when its value is discarded and the call is dropped outright.
    const auto *CE = clang::cast<clang::CallExpr>(E);
    std::optional<HavocAction> action = classifyCall(CE, *_C);
    if (!action)
      return false;
    if (action->mode == HavocAction::Mode::Pointer) {
      bool discarded = false;
      hoistAnchor(CE, discarded);
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

// True if any part of `S` is a call this transform havocs. Paired with
// isSideEffectFree, this is what keeps pruning to statements we ourselves
// made vacuous: an author's own dead expression statement (`x;`) is pure but
// contains nothing of ours, so it survives untouched.
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

// A `for` loop's init clause is a statement, not an expression: either a
// declaration (`for (int i = 0; ...)`, checked via `mutableVars` - the
// caller already derived it from this same `init` via loopLocalVars) or a
// bare expression-statement. `dyn_cast_or_null` folds the null-init case
// (no init clause at all) into the same side-effect-free fallthrough as an
// unreachable non-DeclStmt/non-Expr init - Clang's AST never produces the
// latter for a ForStmt, so the permissive default is never actually hit.
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
      // `node` is a direct child of a block: the statement to hoist above.
      const clang::Stmt *anchor = node.get<clang::Stmt>();
      if (!anchor)
        return nullptr;
      // The call is the whole statement (a bare expression statement) when the
      // anchor, stripped of parens and implicit conversions, is the call itself.
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
    // No handle survives on the block, so nothing needs synthesizing.
    debugLog(4, "[transform] " + where + ": dropped discarded pointer call");
    eraseStmt(E);
    return true;
  }
  if (!anchor) {
    // Not inside a block (e.g. a global initializer): nowhere to hoist. Leaving
    // the call is the only compilable option - the callee is defined in-file.
    debugLog(2, "[transform] " + where + ": pointer call has no statement to hoist above; left as-is");
    return true;
  }

  clang::QualType returnType = E->getCallReturnType(*_C);
  std::string stub = "__hret" + std::to_string(_StubCounter++);
  PointerStorage store = renderPointerStorage(plan, returnType, stub, returnType.getAsString(),
                                              _C->getPrintingPolicy(), /*indent=*/"");

  // indentNewLines matches the hoisted lines to the anchor statement's column.
  _Rewriter.InsertText(anchor->getBeginLoc(), store.decls, /*InsertAfter=*/false,
                       /*indentNewLines=*/true);
  _Rewriter.ReplaceText(E->getSourceRange(), store.arg);

  // A struct tag named only inside a parameter/return type has prototype
  // scope; hoisting it to file scope is needed regardless of whether this
  // particular call site later turns out unreachable - a stray forward
  // declaration is always legal C, so nothing needs to track its liveness.
  if (!plan.fwdDecl.empty())
    _NeededFwdDecls->insert(plan.fwdDecl);
  debugLog(4, "[transform] " + where + ": havocked pointer call -> stack " + stub);
  return true;
}

// Erasing is idempotent: a statement can be reached both as a rewritten call
// and later as a vacuous child of its enclosing block, and re-removing a range
// that has already been removed would confuse the Rewriter's delta bookkeeping.
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
  // Memoized because an enclosing statement re-asks about subtrees its children
  // already answered for. Under post-order every child is cached before its
  // parent asks, so the recursion below stays one level deep in practice.
  return _NoOpMemo.emplace(S, computeNoOp(S)).first->second;
}

// Statement-level vacuity, derived from the statement rather than accumulated
// during traversal, so it is correct whenever it is asked - not only once
// traversal has passed the node.
bool HavocCallsVisitor::computeNoOp(const clang::Stmt *S) const {
  // A statement we could not rewrite anyway must never be called vacuous, or
  // an enclosing block would try to erase a range that is not really there.
  clang::SourceLocation begin = S->getBeginLoc();
  if (begin.isMacroID() || !_C->getSourceManager().isInMainFile(begin))
    return false;

  // An expression statement we hollowed out: what is left of it is pure, and
  // at least one piece of it is our own rewrite. Both halves matter - the
  // second is what stops us from tidying up dead code the author wrote.
  if (const auto *E = clang::dyn_cast<clang::Expr>(S))
    return containsHavocedCall(E) && isSideEffectFree(E, {});

  if (const auto *CS = clang::dyn_cast<clang::CompoundStmt>(S)) {
    for (const clang::Stmt *child : CS->body()) {
      if (!isNoOp(child))
        return false;
    }
    return true; // an empty block is vacuous too
  }

  // For `if`, pruning a dead branch can never change termination. For loops it
  // can: an empty body spinning on a side-effect-free condition
  // (`while (n > 0);`) may hang, and pruning turns that hang into termination -
  // intentional, since such loops are havoc artifacts, not meaningful
  // termination-benchmark content. A condition/increment with a real side
  // effect is kept, since it may be observed after the loop.
  if (const auto *ifS = clang::dyn_cast<clang::IfStmt>(S))
    return isNoOp(ifS->getThen()) && isNoOp(ifS->getElse()) &&
           isSideEffectFree(ifS->getCond(), {});

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
    // A no-op statement sitting directly in this block can simply vanish;
    // unlike the mandatory single-statement slot of an unbraced if/while/do
    // body, nothing needs to be left in its place. NullStmt and an
    // already-empty nested block have nothing further to erase.
    if (clang::isa<clang::NullStmt>(child) || clang::isa<clang::CompoundStmt>(child))
      continue;
    eraseStmt(child);
    eatTrailingSemicolon(_C, _Rewriter, child);
  }
  return true;
}

// What makes an if/loop vacuous now lives entirely in computeNoOp, so this is
// just the erase half: the statement's branches and clauses no longer have to
// be destructured at the call site and passed back in.
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
