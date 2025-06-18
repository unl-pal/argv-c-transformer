#pragma once

#include <clang/AST/ASTConsumer.h>
#include <llvm/Support/raw_ostream.h>

class GenerateCodeConsumer : public clang::ASTConsumer {
public:
  GenerateCodeConsumer(llvm::raw_fd_ostream      &output);

  void HandleTranslationUnit(clang::ASTContext &context);

private:
  llvm::raw_fd_ostream &_Output;
};
