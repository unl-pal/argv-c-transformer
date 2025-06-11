#include "GenerateIncludeAction.hpp"
#include "GenerateIncludeConsumer.hpp"
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <system_error>

GenerateIncludeAction::GenerateIncludeAction(llvm::raw_fd_ostream &output) : _Output(output) {}

std::unique_ptr<clang::ASTConsumer>
GenerateIncludeAction::CreateASTConsumer(clang::CompilerInstance &compiler,
                                         llvm::StringRef          filename) {
  return std::make_unique<GenerateIncludeConsumer>(_Output); // need the compiler?
}

bool GenerateIncludeAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  llvm::outs() << "Begin Source File Action\n";
  bool result = clang::ASTFrontendAction::BeginSourceFileAction(compiler);
  // GenerateIncludeConsumer consumer(_Output);
  // consumer.HandleTranslationUnit(compiler.getASTContext());
  llvm::outs() << "Post Begin Source File Action\n";
  return result;
  // return true;
}

void GenerateIncludeAction::EndSourceFileAction() {
  llvm::outs() << "Ending Source File Action\n";
}

// llvm::raw_fd_ostream GenerateIncludeAction::SetOutput(llvm::StringRef filename, std::error_code ec) {
//   return llvm::raw_fd_ostream(filename, ec);
// }
