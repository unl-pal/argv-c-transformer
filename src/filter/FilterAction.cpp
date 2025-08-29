#include "AddVerifiersConsumerFilter.hpp"
#include "CountingConsumer.hpp"
#include "CountingVisitor.hpp"
#include "FilterAction.hpp"
#include "FilterFunctionsConsumer.hpp"
#include "RemoveConsumer.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/TemplateName.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/AST/Type.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


FilterAction::FilterAction(std::map<std::string, int>      *config,
                           const std::vector<unsigned int> &types,
                           llvm::raw_fd_ostream            &output)
    : _Config(config), _Types(types), _Rewriter(), _Output(output) {
  llvm::outs() << "Created FilterAction" << "\n";
}

std::unique_ptr<clang::ASTConsumer>
FilterAction::CreateASTConsumer(clang::CompilerInstance &compiler,
                                llvm::StringRef          filename) {

  // Only expand directive macros such as pragmas
  // Not in use as it hinders all "include"s and renders benchmark uncompilable
  // compiler.getPreprocessor().SetMacroExpansionOnlyInDirectives();

  llvm::outs() << "Created ASTConsumer" << "\n";
  // Create the AST used for the entire filter action and filter action only
  compiler.createASTContext();

  // All functions to filter through
  std::unordered_map<std::string, CountNodesVisitor::attributes *> *toFilter =
    new std::unordered_map<std::string, CountNodesVisitor::attributes *>();
  // Functions to be removed
  std::vector<std::string> *toRemove = new std::vector<std::string>();
  // __VERIFIER types that will be required
  std::set<clang::QualType> *neededTypes = new std::set<clang::QualType>();
  llvm::outs() << "Created To Filter and Remove Vars" << "\n";

  // TempVector simplifies cast for multiplexor due to compiler attempts to
  // optimize and interpret vector as all one type which breaks the code
  std::vector<std::unique_ptr<clang::ASTConsumer>> tempVector;
  tempVector.emplace_back(std::make_unique<CountingConsumer>(_Types, toFilter));
  tempVector.emplace_back(
    std::make_unique<FilterFunctionsConsumer>(toFilter, toRemove, _Config));
  tempVector.emplace_back(std::make_unique<RemoveConsumer>(_Rewriter, toRemove, neededTypes));
  tempVector.emplace_back(std::make_unique<AddVerifiersConsumerFilter>(_Output, neededTypes, _Rewriter));

  llvm::outs() << "Created Temp Vector" << "\n";

  // Multiplexor of all consumers that will be run over the same AST
  // use move from temp to avoid compiler optimizations that may break the code
  std::unique_ptr<clang::MultiplexConsumer> result =
    std::make_unique<clang::MultiplexConsumer>(std::move(tempVector));

  // Debug statement for when debug levels are implemented
  llvm::outs() << "CreateASTConsumer Method ran on: " << filename << "\n";
  return result;
}

bool FilterAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  llvm::outs() << "Begin Source File Action" << "\n";
  // Sets up the Rewriter before AST actions
  _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  bool result = clang::ASTFrontendAction::BeginSourceFileAction(compiler);
  return result;
}

void FilterAction::EndSourceFileAction() {
  llvm::outs() << "End Source File Action" << "\n";
  // Writes the edited buffer to the new filtered file location
  _Rewriter.getEditBuffer(getCompilerInstance().getSourceManager().getMainFileID()).write(_Output);
}
