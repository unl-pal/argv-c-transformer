#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief ASTConsumer that drives the body-stripping pass over the AST.
 *
 * Skips the visitor entirely if {@code toRemove} is empty. Otherwise
 * constructs a {@code RemoveVisitor} and traverses the translation unit,
 * which replaces the bodies of filtered-out functions with {@code ;}.
 */
class RemoveConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param rewriter      Shared rewriter; edits accumulate here across all consumers.
   * @param toRemove      Names of functions to strip, written by {@code FilterFunctionsConsumer}.
   */
  RemoveConsumer(clang::Rewriter &rewriter, std::shared_ptr<std::vector<std::string>> toRemove);

  /**
   * @brief Entry point called by Clang once the AST is fully parsed.
   *
   * No-ops if {@code toRemove} is empty. Otherwise runs {@code RemoveVisitor}
   * over the full translation unit.
   *
   * @param context  The AST context for this translation unit.
   */
  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  clang::Rewriter &_Rewriter;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
};
