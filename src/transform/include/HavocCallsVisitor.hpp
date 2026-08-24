// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <initializer_list>
#include <set>

/**
 * @brief Havocs every in-file function call so bodies become intraprocedural.
 *
 * Primitive returns -> {@code __VERIFIER_nondet_<type>()}; pointer returns ->
 * a uniquely-named buffer hoisted to the top of the enclosing function plus a
 * comma-expression at the call site that fills it and yields it (char
 * pointees go through {@code __havoc_cstring_fill}, from argv_c_harness.h,
 * for null-termination); void returns dropped; aggregate returns left as-is.
 * Dropped calls are marked no-op, and enclosing loops/branches that become
 * side-effect-free no-ops are pruned.
 */
class HavocCallsVisitor : public clang::RecursiveASTVisitor<HavocCallsVisitor> {
public:
  /**
   * @param C        AST context, used for return-type resolution and source manager access.
   * @param rewriter Shared rewriter for modifying the source buffer.
   */
  HavocCallsVisitor(clang::ASTContext *C, clang::Rewriter &rewriter);

  /**
   * @brief Havocs a call if it should be (in-file, non-library, non-verifier, non-macro).
   * @param E The call expression being visited.
   * @return false to stop traversal, true to continue.
   */
  bool VisitCallExpr(clang::CallExpr *E);

  /**
   * @brief Tracks the enclosing function's hoist point (just inside its
   * opening brace) for the duration of its body, so a havocked pointer call
   * anywhere inside can hoist its buffer declaration there.
   * @param D The function declaration being traversed.
   * @return true to continue traversal.
   */
  bool TraverseFunctionDecl(clang::FunctionDecl *D);

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
   * @brief True for NullStmt and anything previously recorded in _NoOpStmts.
   * @param S The statement to classify, or nullptr.
   */
  bool isNoOp(const clang::Stmt *S) const;

private:
  /**
   * @brief Shared prune rule for if/while/do/for.
   *
   * Erases S and marks it a no-op if every statement in branches is a no-op
   * and cond/init/inc are all side-effect-free (init/inc only apply to for-loops).
   *
   * @param S        The statement to erase if it proves to be a no-op.
   * @param keyLoc   Leading keyword location, used for the main-file/macro guard.
   * @param branches Every branch/body statement that must be a no-op.
   * @param cond     The controlling condition; must be side-effect-free.
   * @param init     A for-loop's init clause, or nullptr.
   * @param inc      A for-loop's increment clause, or nullptr.
   * @return true if the statement was erased.
   */
  bool pruneIfNoOp(clang::Stmt *S, clang::SourceLocation keyLoc,
                   std::initializer_list<const clang::Stmt *> branches, const clang::Expr *cond,
                   const clang::Stmt *init = nullptr, const clang::Expr *inc = nullptr);

  clang::ASTContext *_C;
  clang::Rewriter &_Rewriter;
  std::set<const clang::Stmt *> _NoOpStmts;

  /** @brief Insertion point just inside the current function's opening
   * brace, or invalid outside any function body. Set by TraverseFunctionDecl. */
  clang::SourceLocation _HoistPoint;
  /** @brief Monotonic counter giving every hoisted buffer a unique name within the file. */
  unsigned _HavocCounter = 0;
};
