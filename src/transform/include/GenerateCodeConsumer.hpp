#pragma once

#include <clang/AST/ASTConsumer.h>
#include <llvm/Support/raw_ostream.h>

/**
 * @brief ASTConsumer that launches the visitor to write final source code.
 *
 * Delegates to {@code RegenCodeVisitor} to pretty-print the transformed AST
 * back to C source, writing the result to the provided output stream.
 */
class GenerateCodeConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer, binding the output stream.
   *
   * @param output File stream to write the regenerated source code to.
   */
  GenerateCodeConsumer(llvm::raw_fd_ostream &output);

  /**
   * @brief Launches the visitor to print the code to the output location.
   *
   * @param context The AST context for the translation unit being transformed.
   */
  void HandleTranslationUnit(clang::ASTContext &context);

private:
  llvm::raw_fd_ostream &_Output;
};
