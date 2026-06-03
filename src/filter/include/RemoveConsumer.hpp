#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <set>
#include <string>
#include <vector>

/**
 * @brief ASTConsumer that drives the removal pass over the AST.
 *
 * Skips the visitor entirely if {@code toRemove} is empty. Otherwise
 * constructs a {@code RemoveFuncVisitor} and traverses the translation unit,
 * which deletes function bodies and replaces their call sites with
 * {@code __VERIFIER_nondet_*()} calls, populating {@code neededTypes} for
 * the next consumer ({@code AddVerifiersConsumerFilter}).
 */
class RemoveConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param rewriter      Shared rewriter; edits accumulate here across all consumers.
   * @param toRemove      Names of functions to delete, written by {@code FilterFunctionsConsumer}.
   * @param neededTypes   Output set; visitor adds return types of replaced calls.
   */
  RemoveConsumer(clang::Rewriter &rewriter, std::vector<std::string> *toRemove,
                 std::set<std::string> *neededTypes);

  /**
   * @brief Entry point called by Clang once the AST is fully parsed.
   *
   * No-ops if {@code toRemove} is empty. Otherwise runs {@code RemoveFuncVisitor}
   * over the full translation unit.
   *
   * @param context  The AST context for this translation unit.
   */
  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  clang::Rewriter &_Rewriter;
  std::vector<std::string> *_ToRemove;
  std::set<std::string> *_NeededTypes;
};
