#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <set>
#include <string>
#include <vector>

/**
 * @brief Visitor that removes filtered functions and replaces their call sites.
 *
 * Two passes in one traversal:
 * - {@code VisitFunctionDecl}: deletes the source text of each function in
 *   {@code _ToRemove}, including any attached doc comment.
 * - {@code VisitCallExpr}: replaces calls to removed functions with
 *   {@code __VERIFIER_nondet_<type>()} and records the return type in
 *   {@code _NeededTypes} for {@code AddVerifiersConsumerFilter} to inject
 *   the corresponding extern declaration.
 */
class RemoveVisitor : public clang::RecursiveASTVisitor<RemoveVisitor> {
public:
  /**
   * @brief Constructs the visitor with the shared pipeline state.
   *
   * @param C            AST context, used for comment lookup and type queries.
   * @param rewriter     Shared rewriter; deletions and replacements accumulate here.
   * @param toRemove     Names of functions to remove, from {@code FilterFunctionsConsumer}.
   * @param neededTypes  Output set; populated with return types of replaced calls.
   */
  RemoveVisitor(clang::ASTContext *C, clang::Rewriter &rewriter, std::vector<std::string> *toRemove,
                std::set<std::string> *neededTypes);

  /**
   * @brief Deletes function definitions (and their doc comments) for functions in {@code
   * _ToRemove}.
   *
   * Skips macro-expanded locations (not writable by the Rewriter) and
   * {@code main}. For forward declarations the range is extended by one
   * character to include the trailing semicolon.
   */
  bool VisitFunctionDecl(clang::FunctionDecl *D);

  /**
   * @brief Replaces calls to removed functions with {@code __VERIFIER_nondet_<type>()}.
   *
   * Normalises the return type name (strips spaces, underscores, stars) to
   * build the verifier name. Records the return type in {@code _NeededTypes}.
   */
  bool VisitCallExpr(clang::CallExpr *E);

  /** @brief Uses pre-order traversal (default); post-order left as future option. */
  bool shouldTraversePostOrder();

private:
  clang::ASTContext *_C;
  clang::SourceManager &_Mgr;
  clang::Rewriter &_Rewriter;
  std::vector<std::string> *_ToRemove;
  std::set<std::string> *_NeededTypes;
};
