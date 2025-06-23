#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

class FilterAction : public clang::ASTFrontendAction {
public:
  FilterAction(llvm::raw_fd_ostream &output);

  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef          Filename) override;

  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

  void EndSourceFileAction() override;

  // bool usesPreprocessorOnly() const override;

private:
  llvm::raw_fd_ostream &_Output;
};

