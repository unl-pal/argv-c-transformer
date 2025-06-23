#include "FilterAction.hpp"
#include "CountingConsumer.hpp"
#include "RemoveConsumer.hpp"

#include <clang/AST/TemplateName.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>
#include <vector>


// Constructor for GenerateIncludeAction that sets up the output stream for
// regenerating source code
FilterAction::FilterAction(llvm::raw_fd_ostream &output) : _Output(output) {}

// Overridden function that uses a ConsumerMultiplexer instead of a single
// ASTConsumer to run many consumers, handlers and visitors over the same AST
std::unique_ptr<clang::ASTConsumer>
FilterAction::CreateASTConsumer(clang::CompilerInstance &compiler,
                                         llvm::StringRef          filename) {

  // TODO - Get these from the filterer or set them up or whatever to get them
  // incorporated
  std::vector<unsigned int> Types;
  std::vector<std::string> ToRemove;
  clang::Rewriter Rewriter;

  std::vector<std::unique_ptr<clang::ASTConsumer>> tempVector;
  tempVector.emplace_back(std::make_unique<CountingConsumer>(Types));
  tempVector.emplace_back(std::make_unique<RemoveConsumer>(Rewriter, ToRemove));

  // Multiplexor of all consumers that will be run over the same AST
  std::unique_ptr<clang::MultiplexConsumer> result =
    std::make_unique<clang::MultiplexConsumer>(std::move(tempVector));

  // Debug statement for when debug levels are implemented
  // llvm::outs() << "CreateASTConsumer Method ran on: " << filename << "\n";
  return result;
}

// May be needed or implemented later to force only the preprocessor to run on code
// bool GenerateIncludeAction::usesPreprocessorOnly() const {
//   return 1;
// }

// Function that runs before any of the consumers but after preprocessor steps
bool FilterAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  bool result = clang::ASTFrontendAction::BeginSourceFileAction(compiler);
  return result;
}

// Function that runs after all of the consumers but before the AST is cleaned up
void FilterAction::EndSourceFileAction() {
}
