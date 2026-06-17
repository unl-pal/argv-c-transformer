#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
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
 * Library calls (C stdlib, system headers) and {@code __VERIFIER_*} calls are
 * kept unchanged. Calls inside macro expansions are skipped (no rewritable
 * source range).
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
   * @brief Instructs the visitor to use post-order (depth-first) traversal.
   *
   * @return {@code true} for post-order traversal.
   */
  bool shouldTraversePostOrder();

private:
  clang::ASTContext *_C;
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
};
