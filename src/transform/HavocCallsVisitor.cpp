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
  enum class Mode { Erase, Inline, Pointer, Reject } mode;
  std::string replacement;     // Inline only.
  PointerPlan plan;            // Pointer only.
  clang::CharSourceRange text; // Inline, and Erase inside a macro: the file text to replace.
};

// True if `text` is exactly one call: parens balanced and closing at the end, with no top-level
// `,` or `;`, which would mean the range spans several macro arguments.
bool isSingleCallText(llvm::StringRef text) {
  int depth = 0;
  bool opened = false;
  for (size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '"' || c == '\'') {
      for (++i; i < text.size() && text[i] != c; ++i)
        if (text[i] == '\\') ++i;
      continue;
    }
    if (c == '(') {
      ++depth;
      opened = true;
    } else if (c == ')') {
      if (--depth < 0) return false;
    } else if ((c == ',' || c == ';') && depth == 0) {
      return false;
    }
  }
  return opened && depth == 0 && text.ends_with(")");
}

// The main-file text spelling call E, if one contiguous run of it can be rewritten: the call as
// written, a whole macro use (`CHECK(v)`), its text in a macro argument (`MAX(f(x), 1)`), or its
// text in one macro's #define body. Rewriting a #define body havocs every expansion of it.
std::optional<clang::CharSourceRange> rewritableSpelling(const clang::CallExpr *E,
                                                         clang::ASTContext &C) {
  const clang::SourceManager &mgr = C.getSourceManager();
  const clang::LangOptions &langOpts = C.getLangOpts();
  clang::CharSourceRange range = clang::Lexer::makeFileCharRange(
      clang::CharSourceRange::getTokenRange(E->getSourceRange()), mgr, langOpts);
  if (range.isInvalid()) {
    clang::SourceLocation begin = E->getBeginLoc(), end = E->getEndLoc();
    while (begin.isMacroID() && end.isMacroID()) { // walk out of one macro body at a time
      if (mgr.isMacroArgExpansion(begin) || mgr.isMacroArgExpansion(end) ||
          mgr.getFileID(begin) != mgr.getFileID(end))
        return std::nullopt;
      begin = mgr.getImmediateSpellingLoc(begin);
      end = mgr.getImmediateSpellingLoc(end);
    }
    if (begin.isMacroID() || end.isMacroID()) return std::nullopt;
    range = clang::Lexer::makeFileCharRange(clang::CharSourceRange::getTokenRange(begin, end), mgr,
                                            langOpts);
  }
  if (range.isInvalid() || !mgr.isInMainFile(range.getBegin())) return std::nullopt;

  bool invalid = false;
  llvm::StringRef text = clang::Lexer::getSourceText(range, mgr, langOpts, &invalid);
  if (invalid || !isSingleCallText(text)) return std::nullopt;
  return range;
}

// Decides whether a call is havocked, and into what. nullopt leaves a genuine
// library/verifier call alone; Mode::Reject also leaves the call's text alone
// but marks it unhavockable, so the caller taints the enclosing function
// rather than emitting a real call into an unsound benchmark. Stateless, so
// callable repeatedly for the same call.
std::optional<HavocAction> classifyCall(const clang::CallExpr *E, clang::ASTContext &C) {
  clang::SourceManager &mgr = C.getSourceManager();
  if (!mgr.isInMainFile(E->getExprLoc())) return std::nullopt;

  if (const clang::FunctionDecl *callee = E->getDirectCallee()) {
    if (callee->getIdentifier() && callee->getName().starts_with("__VERIFIER_"))
      return std::nullopt;
    if (callee->isImplicit() && callee->getBuiltinID() != 0) // e.g. __builtin_expect: no header
      return std::nullopt;
    if (!callee->isImplicit() &&
        !mgr.isInMainFile(callee->getLocation()) && mgr.isInSystemHeader(callee->getLocation()))
      return std::nullopt;
  }

  clang::QualType returnType = E->getCallReturnType(C);
  if (returnType.isNull() || returnType.getTypePtrOrNull() == nullptr) return std::nullopt;

  HavocAction action{HavocAction::Mode::Reject, "", {}, {}};
  if (returnType->isVoidType()) {
    action.mode = HavocAction::Mode::Erase;
  } else if (std::optional<std::string> suffix = verifierSuffixForType(returnType)) {
    action.mode = HavocAction::Mode::Inline;
    action.replacement = "__VERIFIER_nondet_" + *suffix + "()";
  } else if (returnType->isAnyPointerType()) {
    PointerPlan plan = planPointer(returnType, mgr); // storage/placement are the caller's job
    if (plan.viable) {
      action.mode = HavocAction::Mode::Pointer;
      action.plan = plan;
    }
  }
  // Otherwise an aggregate return: no expression-position nondet equivalent. Reject rather
  // than leave the real call in place — the callee may have no definition in
  // the output at all (a header-closure prototype, or a filter-rejected
  // sibling), and even when it does, leaving the call real breaks the
  // intraprocedural guarantee every other havocked call gives.
  if (action.mode == HavocAction::Mode::Reject) return action;

  if (!E->getBeginLoc().isMacroID() && !E->getEndLoc().isMacroID()) {
    action.text = clang::CharSourceRange::getTokenRange(E->getSourceRange());
    return action;
  }
  // Inside a macro: rewrite where the call is spelled. Pointer storage has no statement there
  // to hoist above.
  std::optional<clang::CharSourceRange> spelling = rewritableSpelling(E, C);
  if (!spelling || action.mode == HavocAction::Mode::Pointer)
    return HavocAction{HavocAction::Mode::Reject, "", {}, {}};
  action.text = *spelling;
  return action;
}

std::set<const clang::VarDecl *> loopLocalVars(const clang::Stmt *init) {
  std::set<const clang::VarDecl *> vars;
  if (const auto *declStmt = clang::dyn_cast_or_null<clang::DeclStmt>(init)) {
    for (const clang::Decl *D : declStmt->decls()) {
      if (const auto *VD = clang::dyn_cast<clang::VarDecl>(D)) vars.insert(VD);
    }
  }
  return vars;
}

const clang::VarDecl *referencedVar(const clang::Expr *E) {
  if (!E) return nullptr;
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
  if (!presumed.isValid()) return "<unknown>";
  return std::string(presumed.getFilename()) + ":" + std::to_string(presumed.getLine());
}

} // namespace

// Anything not explicitly recognized is treated as side-effecting.
// `mutableVars`: a for-loop's own init-declared variables.
bool HavocCallsVisitor::isSideEffectFree(
    const clang::Expr *E, const std::set<const clang::VarDecl *> &mutableVars) const {
  if (!E) return true;
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
    if (!action || action->mode == HavocAction::Mode::Reject)
      return false; // a rejected call is a real, unreplaced call: not pure
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
    return isSideEffectFree(AS->getBase(), mutableVars) &&
           isSideEffectFree(AS->getIdx(), mutableVars);
  }
  default:
    return false;
  }
}

bool HavocCallsVisitor::containsHavocedCall(const clang::Stmt *S) const {
  if (!S) return false;
  if (const auto *CE = clang::dyn_cast<clang::CallExpr>(S)) {
    std::optional<HavocAction> action = classifyCall(CE, *_C);
    if (action && action->mode != HavocAction::Mode::Reject) return true;
  }
  for (const clang::Stmt *child : S->children()) {
    if (containsHavocedCall(child)) return true;
  }
  return false;
}

// init is a declaration or a bare expression-statement, or null if omitted.
bool HavocCallsVisitor::isInitSideEffectFree(
    const clang::Stmt *init, const std::set<const clang::VarDecl *> &mutableVars) const {
  for (const clang::VarDecl *varDecl : mutableVars) {
    if (!isSideEffectFree(varDecl->getInit(), mutableVars)) return false;
  }
  if (const auto *E = clang::dyn_cast_or_null<clang::Expr>(init))
    return isSideEffectFree(E, mutableVars);
  return true;
}

HavocCallsVisitor::HavocCallsVisitor(clang::ASTContext *C,
                                     std::shared_ptr<std::set<std::string>> neededFwdDecls,
                                     clang::Rewriter &rewriter)
    : _C(C), _NeededFwdDecls(neededFwdDecls), _Rewriter(rewriter) {};

bool HavocCallsVisitor::TraverseCallExpr(clang::CallExpr *E) {
  std::optional<HavocAction> action = classifyCall(E, *_C);
  if (action && action->mode != HavocAction::Mode::Reject)
    return VisitCallExpr(E); // rewrites E outright; its arguments' text goes with it
  return RecursiveASTVisitor<HavocCallsVisitor>::TraverseCallExpr(E);
}

bool HavocCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  std::optional<HavocAction> action = classifyCall(E, *_C);
  if (!action) return true;

  std::string where = locString(_C->getSourceManager(), E->getExprLoc());
  if (action->mode == HavocAction::Mode::Reject) {
    if (const clang::FunctionDecl *enclosing = enclosingFunction(E)) {
      debugLog(2, "[transform] " + where + ": unhavockable call taints " +
                      enclosing->getNameAsString());
      _Tainted.insert(enclosing);
    }
    return true;
  }
  if (action->mode == HavocAction::Mode::Erase) {
    debugLog(4, "[transform] " + where + ": dropped void call");
    if (E->getBeginLoc().isMacroID() || E->getEndLoc().isMacroID())
      dropMacroSpelledCall(E, action->text);
    else
      eraseStmt(E);
    return true;
  }
  if (action->mode == HavocAction::Mode::Pointer) return havocPointerReturn(E, action->plan, where);

  clang::SourceManager &mgr = _C->getSourceManager();
  if (!_RewrittenSpellings.insert(mgr.getFileOffset(action->text.getBegin())).second)
    return true; // text shared by several expansions (a #define body, a twice-used macro arg)
  debugLog(4, "[transform] " + where + ": havocked call -> " + action->replacement);
  _Rewriter.ReplaceText(action->text, action->replacement);
  return true;
}

// Walks parents to the CallExpr's nearest enclosing FunctionDecl; every call
// site is inside exactly one (file-scope initializers can't contain calls).
const clang::FunctionDecl *HavocCallsVisitor::enclosingFunction(const clang::CallExpr *E) const {
  clang::DynTypedNode node = clang::DynTypedNode::create(*E);
  while (true) {
    clang::DynTypedNodeList parents = _C->getParents(node);
    if (parents.empty())
      return nullptr;
    const clang::DynTypedNode &parent = parents[0];
    if (const auto *func = parent.get<clang::FunctionDecl>())
      return func;
    node = parent;
  }
}

const clang::Stmt *HavocCallsVisitor::hoistAnchor(const clang::CallExpr *E, bool &discarded) const {
  discarded = false;
  clang::DynTypedNode node = clang::DynTypedNode::create(*E);
  while (true) {
    clang::DynTypedNodeList parents = _C->getParents(node);
    if (parents.empty()) return nullptr;
    const clang::DynTypedNode &parent = parents[0];
    if (parent.get<clang::CompoundStmt>()) {
      const clang::Stmt *anchor = node.get<clang::Stmt>();
      if (!anchor) return nullptr;
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
    debugLog(2,
             "[transform] " + where + ": pointer call has no statement to hoist above; left as-is");
    return true;
  }

  clang::QualType returnType = E->getCallReturnType(*_C);
  std::string stub = "__hret" + std::to_string(_StubCounter++);
  PointerStorage store = renderPointerStorage(plan, returnType, stub, returnType.getAsString(),
                                              /*indent=*/"");

  _Rewriter.InsertText(anchor->getBeginLoc(), store.decls, /*InsertAfter=*/false,
                       /*indentNewLines=*/true);
  _Rewriter.ReplaceText(E->getSourceRange(), store.arg);

  if (!plan.fwdDecl.empty()) _NeededFwdDecls->insert(plan.fwdDecl);
  debugLog(4, "[transform] " + where + ": havocked pointer call -> stack " + stub);
  return true;
}

void HavocCallsVisitor::dropMacroSpelledCall(const clang::CallExpr *E,
                                             clang::CharSourceRange text) {
  clang::SourceManager &mgr = _C->getSourceManager();
  if (!_RewrittenSpellings.insert(mgr.getFileOffset(text.getBegin())).second) return;
  bool discarded = false;
  hoistAnchor(E, discarded);
  if (!discarded) { // the value feeds an expression, e.g. (LOG(v), 1)
    _Rewriter.ReplaceText(text, "((void)0)");
    return;
  }
  _Rewriter.RemoveText(text);
  const char *after = mgr.getCharacterData(text.getEnd());
  unsigned skip = 0;
  while (after[skip] == ' ' || after[skip] == '\t') ++skip;
  if (after[skip] == ';') _Rewriter.RemoveText(text.getEnd().getLocWithOffset(skip), 1);
}

// Idempotent: re-removing an already-erased range confuses the Rewriter's delta bookkeeping.
void HavocCallsVisitor::eraseStmt(const clang::Stmt *S) {
  if (!_ErasedStmts.insert(S).second) return;
  _Rewriter.ReplaceText(S->getSourceRange(), "");
}

bool HavocCallsVisitor::isNoOp(const clang::Stmt *S) const {
  if (!S || clang::isa<clang::NullStmt>(S)) return true;
  auto cached = _NoOpMemo.find(S);
  if (cached != _NoOpMemo.end()) return cached->second;
  return _NoOpMemo.emplace(S, computeNoOp(S))
      .first->second; // an enclosing statement re-asks about its children
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
      if (!isNoOp(child)) return false;
    }
    return true;
  }

  if (const auto *ifS = clang::dyn_cast<clang::IfStmt>(S))
    return isNoOp(ifS->getThen()) && isNoOp(ifS->getElse()) && isSideEffectFree(ifS->getCond(), {});

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
    if (!isNoOp(child)) continue;
    if (clang::isa<clang::NullStmt>(child) ||
        clang::isa<clang::CompoundStmt>(child)) // nothing further to erase
      continue;
    eraseStmt(child);
    eatTrailingSemicolon(_C, _Rewriter, child);
  }
  return true;
}

void HavocCallsVisitor::pruneIfNoOp(clang::Stmt *S, clang::SourceLocation keyLoc) {
  if (!isNoOp(S)) return;
  debugLog(3,
           "[transform] " + locString(_C->getSourceManager(), keyLoc) + ": pruned no-op statement");
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
