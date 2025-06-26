#pragma once

#include <clang/AST/ASTConsumer.h>
#include <llvm/Support/raw_ostream.h>

class GenerateIncludeConsumer : public clang::ASTConsumer {
public:
	/// Uses the preprocessor to identify include statements required for the code
	GenerateIncludeConsumer(llvm::raw_fd_ostream &output);

  /// Used to insure the creation of the AST and to provide consistancy with the
  /// other consumers used
  virtual void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
	llvm::raw_fd_ostream &_Output;
};
