#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/ADT/StringRef.h>
#include <memory>
#include <system_error>

class GenerateIncludeAction : public clang::ASTFrontendAction {
public:
  GenerateIncludeAction(llvm::raw_fd_ostream &output);
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &compiler,
                    llvm::StringRef          filename) override;

  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

  void EndSourceFileAction() override;

  // void SetOutput(llvm::StringRef filename, std::error_code ec);

private:
  llvm::raw_fd_ostream &_Output;
};

