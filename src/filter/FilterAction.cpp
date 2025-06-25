#include "FilterAction.hpp"
#include "CountingConsumer.hpp"
#include "RemoveConsumer.hpp"

#include <CountingVisitor.hpp>
#include <FilterFunctionsConsumer.hpp>
#include <clang/AST/ASTContext.h>
#include <clang/AST/TemplateName.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <unordered_map>
#include <vector>


// Constructor for GenerateIncludeAction that sets up the output stream for
// regenerating source code
FilterAction::FilterAction(std::map<std::string, int>      *config,
                           const std::vector<unsigned int> &types,
                           llvm::raw_fd_ostream            &output)
    : _Config(config), _Types(types), _Rewriter(), _Output(output) {
  llvm::outs() << "Created FilterAction" << "\n";
}

// Overridden function that uses a ConsumerMultiplexer instead of a single
// ASTConsumer to run many consumers, handlers and visitors over the same AST
std::unique_ptr<clang::ASTConsumer>
FilterAction::CreateASTConsumer(clang::CompilerInstance &compiler,
                                llvm::StringRef          filename) {

  llvm::outs() << "Created ASTConsumer" << "\n";
  compiler.createASTContext();
  // _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  // llvm::outs() << "Created Rewriter" << "\n";

  std::unordered_map<std::string, CountNodesVisitor::attributes *> *toFilter =
    new std::unordered_map<std::string, CountNodesVisitor::attributes *>();
  std::vector<std::string> *toRemove = new std::vector<std::string>();
  llvm::outs() << "Created To Filter and Remove Vars" << "\n";

  std::vector<std::unique_ptr<clang::ASTConsumer>> tempVector;
  tempVector.emplace_back(std::make_unique<CountingConsumer>(_Types, toFilter));
  tempVector.emplace_back(
    std::make_unique<FilterFunctionsConsumer>(toFilter, toRemove, _Config));
  tempVector.emplace_back(std::make_unique<RemoveConsumer>(_Rewriter, toRemove));

  llvm::outs() << "Created Temp Vector" << "\n";

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
  llvm::outs() << "Begin Source File Action" << "\n";
  _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  bool result = clang::ASTFrontendAction::BeginSourceFileAction(compiler);
  return result;
}

// Function that runs after all of the consumers but before the AST is cleaned up
void FilterAction::EndSourceFileAction() {
  llvm::outs() << "End Source File Action" << "\n";
  // _Rewriter.overwriteChangedFiles();
  _Rewriter.getEditBuffer(getCompilerInstance().getSourceManager().getMainFileID()).write(_Output);
}
