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
 *
 * After traversal, any function whose body collapsed entirely to no-ops
 * (e.g. a void wrapper whose only calls were themselves dropped) is stripped
 * to a bare declaration and its name recorded in {@code noOpFunctions}, so
 * {@code MainGenConsumer} does not bother harnessing it.
 */
class HavocCallsConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param neededSuffixes Output set; verifier suffixes needed by havoc
   *        replacements are inserted here.
   * @param noOpFunctions  Output set; names of functions whose bodies
   *        collapsed entirely to no-ops are inserted here.
   * @param rewriter       Shared rewriter for modifying the source buffer.
   */
  HavocCallsConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                     std::shared_ptr<std::set<std::string>> noOpFunctions,
                     clang::Rewriter &rewriter);

  /**
   * @brief Launches {@code HavocCallsVisitor}, populates the suffix set, and
   * strips any function whose body ended up entirely a no-op.
   *
   * @param Context The AST context for the translation unit being transformed.
   */
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  std::shared_ptr<std::set<std::string>> _NoOpFunctions;
  clang::Rewriter &_Rewriter;
};
