// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "HavocPolicy.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <map>
#include <memory>
#include <set>
#include <string>

/**
 * @brief Havocs every in-file function call so bodies become intraprocedural.
 *
 * Primitive returns -> {@code __VERIFIER_nondet_<type>()} inline; pointer
 * returns -> stack storage filled with {@code __VERIFIER_nondet_memory},
 * hoisted above the enclosing statement (see {@code renderPointerStorage}); void
 * and discarded pointer returns dropped; aggregate returns left as-is. Dropped
 * calls are marked no-op, and enclosing loops/branches that become
 * side-effect-free no-ops are pruned.
 */
class HavocCallsVisitor : public clang::RecursiveASTVisitor<HavocCallsVisitor> {
public:
  /**
   * @param C              AST context, used for return-type resolution and source manager access.
   * @param neededSuffixes Output set; verifier suffixes and havoc helper markers are inserted here.
   * @param rewriter       Shared rewriter for modifying the source buffer.
   */
  HavocCallsVisitor(clang::ASTContext *C, std::shared_ptr<std::set<std::string>> neededSuffixes,
                    clang::Rewriter &rewriter);

  /**
   * @brief Havocs a call if it should be (in-file, non-library, non-verifier, non-macro).
   * @param E The call expression being visited.
   * @return false to stop traversal, true to continue.
   */
  bool VisitCallExpr(clang::CallExpr *E);

  /**
   * @brief Marks an empty or all-no-op compound statement as a no-op.
   * @param S The compound statement being visited.
   * @return true to continue traversal.
   */
  bool VisitCompoundStmt(clang::CompoundStmt *S);

  /**
   * @brief Prunes an if statement whose branches are all no-ops and condition is side-effect-free. See pruneIfNoOp.
   * @param S The if statement being visited.
   * @return true to continue traversal.
   */
  bool VisitIfStmt(clang::IfStmt *S);

  /**
   * @brief Prunes a while loop whose body is a no-op and condition side-effect-free.
   * @param S The while statement being visited.
   * @return true to continue traversal.
   */
  bool VisitWhileStmt(clang::WhileStmt *S);

  /**
   * @brief Same rule as VisitWhileStmt, for do/while loops.
   * @param S The do statement being visited.
   * @return true to continue traversal.
   */
  bool VisitDoStmt(clang::DoStmt *S);

  /**
   * @brief Same rule as VisitWhileStmt, for for loops.
   *
   * Init and increment clauses must also be side-effect-free; mutations of
   * the loop's own init-declared variables don't count, since they can't be
   * observed after the loop.
   *
   * @param S The for statement being visited.
   * @return true to continue traversal.
   */
  bool VisitForStmt(clang::ForStmt *S);

  /** @brief Post-order traversal so a statement's children are classified before it is. @return true. */
  bool shouldTraversePostOrder();

  /**
   * @brief Publishes the verifier suffixes of every call that survived, into
   *        the shared neededSuffixes set. Call once, after TraverseDecl.
   *
   * A call's suffix cannot be published when the call is rewritten: an
   * enclosing statement may still turn out to be vacuous and erase it, and a
   * suffix published for an erased call leaves a dangling extern declaration
   * in the output. Buffering until traversal ends is exact and needs no
   * reparse, since every erase decision is made by this visitor.
   */
  void finalizeSuffixes();

  /**
   * @brief True if S contributes nothing once havocking is applied.
   *
   * Derived structurally from S (see computeNoOp), so it holds whenever it is
   * asked rather than only after traversal has passed the node. An expression
   * statement qualifies only if it is side-effect-free *and* contains a call
   * this transform havocs, so dead code the author wrote is never touched -
   * only what we hollowed out.
   *
   * @param S The statement to classify, or nullptr.
   */
  bool isNoOp(const clang::Stmt *S) const;

private:
  /**
   * @brief Erases a statement's text, at most once per statement, and discards
   *        the pending suffixes of every havocked call it contains.
   * @param S The statement to remove from the output buffer.
   */
  void eraseStmt(const clang::Stmt *S);

  /**
   * @brief Forgets the pending suffixes of every havocked call within S.
   * @param S Root of the subtree being erased, or nullptr.
   */
  void dropPendingIn(const clang::Stmt *S);

  /**
   * @brief True if E can be deleted without changing observable behaviour.
   *
   * Havocked calls count as pure - the transform being intraprocedural, a
   * havocked call contributes only its return value - so an expression made
   * entirely of them and of pure operands is removable. Calls left alone
   * (system calls, aggregate returns, non-viable pointer plans) are not.
   *
   * @param E           Expression to check, or nullptr (vacuously true).
   * @param mutableVars Variables whose mutation is unobservable, typically a
   *                    for-loop's own init-declared variables; empty elsewhere.
   */
  bool isSideEffectFree(const clang::Expr *E,
                        const std::set<const clang::VarDecl *> &mutableVars) const;

  /**
   * @brief isSideEffectFree for a for-loop's init clause, which may be a
   *        declaration rather than an expression.
   * @param init        The init clause, or nullptr.
   * @param mutableVars As in isSideEffectFree.
   */
  bool isInitSideEffectFree(const clang::Stmt *init,
                            const std::set<const clang::VarDecl *> &mutableVars) const;

  /**
   * @brief True if any call within S is one this transform havocs.
   * @param S Subtree to search, or nullptr.
   */
  bool containsHavocedCall(const clang::Stmt *S) const;

  /**
   * @brief Shared prune rule for if/while/do/for: erases S if it is a no-op.
   * @param S      The statement to erase if isNoOp accepts it.
   * @param keyLoc Leading keyword location, used only for debug logging.
   */
  void pruneIfNoOp(clang::Stmt *S, clang::SourceLocation keyLoc);

  /**
   * @brief Havocs a pointer-returning call by hoisting stack storage above the
   *        enclosing statement and substituting it for the call.
   *
   * A pointer cannot be synthesized as a bare expression the way a primitive
   * nondet can, so the storage (@ref renderPointerStorage) is emitted as
   * statements before the call's enclosing statement, and the call is replaced
   * by a reference to it. A call whose value is discarded needs no storage and
   * is dropped like a void call; a call with no block to hoist into (not inside
   * a compound statement) is left as-is, the only compilable fallback.
   *
   * @param E     The pointer-returning call.
   * @param plan  Its viable pointer plan.
   * @param where Preformatted "file:line" for debug logging.
   * @return true to continue traversal.
   */
  bool havocPointerReturn(clang::CallExpr *E, const PointerPlan &plan, const std::string &where);

  /**
   * @brief The statement a pointer call's storage must be hoisted above.
   *
   * Walks parents to the nearest node that is a direct child of a compound
   * statement. Sets @p discarded when the call is that statement's whole value
   * (a bare expression statement), meaning the value is unused.
   *
   * @param E         The call to locate.
   * @param discarded Out: true if the call's return value is unused.
   * @return The enclosing statement, or nullptr if the call is not inside a
   *         compound statement (no safe place to hoist).
   */
  const clang::Stmt *hoistAnchor(const clang::CallExpr *E, bool &discarded) const;

  /**
   * @brief Computes statement-level vacuity structurally; isNoOp memoizes it.
   * @param S The statement to classify; never null.
   */
  bool computeNoOp(const clang::Stmt *S) const;

  clang::ASTContext *_C;
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
  /** Memoized computeNoOp results; mutable so the const isNoOp can fill it. */
  mutable std::map<const clang::Stmt *, bool> _NoOpMemo;
  /** Statements already removed from the buffer, so eraseStmt stays idempotent. */
  std::set<const clang::Stmt *> _ErasedStmts;
  /** Suffixes owed by each havocked call, held until the call is known to survive. */
  std::map<const clang::Expr *, std::set<std::string>> _PendingSuffixes;
  /** Names every hoisted pointer-return stub across the TU, keeping them unique. */
  unsigned _StubCounter = 0;
};
