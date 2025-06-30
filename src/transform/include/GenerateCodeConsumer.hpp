#pragma once

#include <clang/AST/ASTConsumer.h>
#include <llvm/Support/raw_ostream.h>

class GenerateCodeConsumer : public clang::ASTConsumer {
public:
  /// Consumer used to launch the visitor that will write all the final source code
  GenerateCodeConsumer(llvm::raw_fd_ostream      &output);

  /// Launches the visitor to print the code supplying the output location to write to
  void HandleTranslationUnit(clang::ASTContext &context);

private:
  llvm::raw_fd_ostream &_Output;
};
