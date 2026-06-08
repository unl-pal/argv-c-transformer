#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <string>

/**
 * @brief ASTConsumer that injects {@code extern __VERIFIER_nondet_*} declarations.
 *
 * Reads the set of verifier type name suffixes populated by {@code RemoveVisitor}
 * and delegates to {@code AddVerifiersVisitorFilter} to insert the corresponding
 * extern declarations before the first node in the file.
 */
class AddVerifiersConsumerFilter : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param output       Destination file stream for the filtered output.
   * @param neededTypes  Set of verifier name suffixes (e.g. "int", "uint") to declare.
   * @param rewriter     Shared rewriter; declarations are inserted here.
   */
  AddVerifiersConsumerFilter(llvm::raw_fd_ostream &output, std::set<std::string> *neededTypes,
                             clang::Rewriter &rewriter);

  /**
   * @brief Entry point called by Clang once the AST is fully parsed.
   *
   * No-ops if {@code neededTypes} is empty. Otherwise runs
   * {@code AddVerifiersVisitorFilter} to inject the declarations.
   *
   * @param context  The AST context for this translation unit.
   */
  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  llvm::raw_fd_ostream &_Output;
  std::set<std::string> *_NeededTypes;
  clang::Rewriter &_Rewriter;
};
