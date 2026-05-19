#pragma once

#include <clang/Frontend/FrontendAction.h>

// FrontendAction is the action being performed on the AST that will then call
// one of the visitors to do the work
class Action : public clang::ASTFrontendAction {
public:
  // This creates the consumer but does that mean it is also creating the AST?
  // TODO - LEARN
  // The consumer life span seems to be the same as the Translation Unit Decl so
  // this could be the spot to handle the regeneration of the AST? Figure out
  // when the generation officially occurs and what the trigger is as well as
  // when it is deleted
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef          Filename) override;

  // TODO - LEARN
  // What is the default of this that we are overriding?
  bool BeginSourceFileAction(clang::CompilerInstance &Compiler) override;

  // TODO - LEARN
  // What is the default of this that we are overriding?
  void EndSourceFileAction() override;
};
