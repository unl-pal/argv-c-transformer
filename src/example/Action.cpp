#include "include/Action.hpp"
#include "include/ConsumerVisitor.hpp"

std::unique_ptr<clang::ASTConsumer>
Action::CreateASTConsumer(clang::CompilerInstance &Compiler,
                          llvm::StringRef          Filename) {

  return std::make_unique<ConsumerVisitor>();
}

bool Action::BeginSourceFileAction(clang::CompilerInstance &Compiler) {
  llvm::outs() << "Begin Source File Action\n";
  bool result = clang::ASTFrontendAction::BeginSourceFileAction(Compiler);
  llvm::outs() << "Ran - Begin Source File Action\n";
  return result;
}

void Action::EndSourceFileAction() {
  clang::ASTFrontendAction::EndSourceFileAction();
  llvm::outs() << "End Source File Action\n";
}
