#pragma once

#include <clang/AST/ASTConsumer.h>
#include <llvm/Support/raw_ostream.h>

class GenerateIncludeConsumer : public clang::ASTConsumer {
public:
	GenerateIncludeConsumer(llvm::raw_fd_ostream &output);
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
	llvm::raw_fd_ostream &_Output;
};
