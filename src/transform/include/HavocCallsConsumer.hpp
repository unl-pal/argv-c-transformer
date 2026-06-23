#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

/**
 * @brief ASTConsumer that havocs every in-file function call.
 *
 * Wraps {@code HavocCallsVisitor} and drives the traversal. After the visitor
 * runs, every call to a function declared in this file has been replaced with
 * a nondeterministic value of the appropriate return type, making each function
 * body intraprocedural. The set of verifier suffixes used is recorded in
 * {@code neededSuffixes} for {@code AddVerifiersConsumer} to declare.
 */
class HavocCallsConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param neededSuffixes Output set; verifier suffixes needed by havoc
   *        replacements are inserted here.
   * @param rewriter       Shared rewriter for modifying the source buffer.
   */
  HavocCallsConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                     clang::Rewriter &rewriter);

  /**
   * @brief Launches {@code HavocCallsVisitor} and populates the suffix set.
   *
   * @param Context The AST context for the translation unit being transformed.
   */
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
};
