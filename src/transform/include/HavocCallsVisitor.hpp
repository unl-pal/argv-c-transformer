#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <initializer_list>
#include <memory>
#include <set>
#include <string>

/**
 * @brief RecursiveASTVisitor that havocs every function call to make bodies intraprocedural.
 *
 * Replaces each {@code CallExpr} to a function declared in the main file with a
 * nondeterministic value of its return type:
 * - Primitive returns → {@code __VERIFIER_nondet_<type>()}
 * - Non-function-pointer returns → {@code __havoc_block(128)} or
 *   {@code __havoc_cstring(128)} (for {@code char *})
 * - {@code void} returns → call is dropped
 * - Aggregate returns (structs, unions) → left as-is
 *
 * Dropped calls are marked as no-ops and enclosures checked for removal
 * so that if any loops or branches are side effect free and contain only
 * no-ops they are removed
 */
class HavocCallsVisitor : public clang::RecursiveASTVisitor<HavocCallsVisitor> {
public:
  /**
   * @brief Constructs the visitor with the AST context and shared pipeline state.
   *
   * @param C              AST context, used for return-type resolution and
   *                       source manager access.
   * @param neededSuffixes Output set; verifier suffixes and havoc helper markers
   *                       are inserted here.
   * @param rewriter       Shared rewriter for modifying the source buffer.
   */
  HavocCallsVisitor(clang::ASTContext *C, std::shared_ptr<std::set<std::string>> neededSuffixes,
                    clang::Rewriter &rewriter);

  /**
   * @brief Initializes the traversal from the translation unit root.
   *
   * @param D The translation unit declaration to traverse.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  virtual bool VisitTranslationUnit(clang::TranslationUnitDecl *D);

  /**
   * @brief Default visit function for all declaration nodes.
   *
   * @param D The declaration being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  virtual bool VisitDecl(clang::Decl *D);

  /**
   * @brief Replaces in-file call expressions with nondeterministic values.
   *
   * This is the primary visitor method. Determines the callee, checks whether
   * the call should be havocked (in-file, non-library, non-verifier, non-macro),
   * and replaces it based on its return type.
   *
   * @param E The call expression being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  virtual bool VisitCallExpr(clang::CallExpr *E);

  /**
   * @brief Marks an empty compound statement, or one whose entire body is
   * made up of no-op statements, as itself a no-op.
   *
   * @param S The compound statement being visited.
   * @return {@code true} to continue traversal.
   */
  virtual bool VisitCompoundStmt(clang::CompoundStmt *S);

  /**
   * @brief Erases an {@code if} statement whose branches are all no-ops and
   * whose condition is side-effect-free, then marks it as a no-op so removal
   * propagates to enclosing statements. See {@code pruneIfNoOp}.
   *
   * @param S The if statement being visited.
   * @return {@code true} to continue traversal.
   */
  virtual bool VisitIfStmt(clang::IfStmt *S);

  /**
   * @brief Erases a {@code while} loop whose body is a no-op and whose
   * condition is side-effect-free. See {@code pruneIfNoOp}.
   *
   * @param S The while statement being visited.
   * @return {@code true} to continue traversal.
   */
  virtual bool VisitWhileStmt(clang::WhileStmt *S);

  /**
   * @brief Same pruning rule as {@code VisitWhileStmt}, for {@code do}/{@code while} loops.
   *
   * @param S The do statement being visited.
   * @return {@code true} to continue traversal.
   */
  virtual bool VisitDoStmt(clang::DoStmt *S);

  /**
   * @brief Same pruning rule as {@code VisitWhileStmt}, for {@code for} loops.
   *
   * Additionally requires the init clause (a bare expression, or a
   * declaration with a side-effect-free initializer) and the increment
   * clause to be side-effect-free.
   *
   * @param S The for statement being visited.
   * @return {@code true} to continue traversal.
   */
  virtual bool VisitForStmt(clang::ForStmt *S);

  /**
   * @brief Instructs the visitor to use post-order (depth-first) traversal.
   * @return {@code true} for post-order traversal.
   */
  bool shouldTraversePostOrder();

  /**
   * @brief Whether a statement performs no observable operation.
   *
   * True for {@code NullStmt}, and for any statement previously recorded in
   * {@code _NoOpStmts} (dropped void calls, empty/all-no-op compound
   * statements, erased if-statements). A null statement pointer (e.g. an
   * absent {@code else} branch) counts as a no-op. Exposed so callers (e.g.
   * {@code HavocCallsConsumer}) can check whether a whole function body
   * collapsed to nothing once traversal is complete.
   *
   * @param S The statement to classify, or {@code nullptr}.
   */
  bool isNoOp(const clang::Stmt *S) const;

private:
  /**
   * @brief Shared prune rule for if/while/do/for.
   *
   * If every statement in {@code branches} is a no-op and {@code cond},
   * {@code init}, and {@code inc} are all side-effect-free (the latter two
   * only meaningful for a for-loop and default to trivially-safe null), the
   * whole statement {@code S} is erased from the source and marked a no-op.
   *
   * @param S        The statement to erase if it proves to be a no-op.
   * @param keyLoc    The statement's leading keyword location, used for the
   *                   main-file/macro guard (e.g. {@code getIfLoc()}).
   * @param branches  Every branch/body statement that must be a no-op.
   * @param cond      The controlling condition; must be side-effect-free.
   * @param init      A for-loop's init clause, or {@code nullptr}.
   * @param inc       A for-loop's increment clause, or {@code nullptr}.
   * @return {@code true} if the statement was erased.
   */
  bool pruneIfNoOp(clang::Stmt *S, clang::SourceLocation keyLoc,
                   std::initializer_list<const clang::Stmt *> branches, const clang::Expr *cond,
                   const clang::Stmt *init = nullptr, const clang::Expr *inc = nullptr);

  clang::ASTContext *_C;
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
  std::set<const clang::Stmt *> _NoOpStmts;
};
