#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

class FilterAction : public clang::ASTFrontendAction {
public:
// Constructor for GenerateIncludeAction that sets up the output stream for
// regenerating source code
  FilterAction(std::map<std::string, int>      *config,
               const std::vector<unsigned int> &types,
               llvm::raw_fd_ostream            &output);

// Overridden function that uses a ConsumerMultiplexer instead of a single
// ASTConsumer to run many consumers, handlers and visitors over the same AST
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef          Filename) override;

// Function that runs before any of the consumers but after preprocessor steps
  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

// Function that runs after all of the consumers but before the AST is cleaned up
  void EndSourceFileAction() override;

  // bool usesPreprocessorOnly() const override;

private:
  std::map<std::string, int> *_Config;
  const std::vector<unsigned int> &_Types; 
  clang::Rewriter _Rewriter;
  llvm::raw_fd_ostream &_Output;
};

