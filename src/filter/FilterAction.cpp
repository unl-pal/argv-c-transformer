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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

FilterAction::FilterAction(std::map<std::string, int> *config,
                           const std::vector<unsigned int> &types, llvm::raw_fd_ostream &output)
    : _Config(config), _Types(types), _Rewriter(), _Output(output) {}

std::unique_ptr<clang::ASTConsumer>
FilterAction::CreateASTConsumer(clang::CompilerInstance &compiler,
                                llvm::StringRef /*filename*/) {
  compiler.createASTContext();

  // TODO: these three are leaked — fix when consumer signatures accept shared_ptr
  std::unordered_map<std::string, CountingVisitor::attributes *> *toFilter =
      new std::unordered_map<std::string, CountingVisitor::attributes *>();
  std::vector<std::string> *toRemove = new std::vector<std::string>();
  std::set<std::string> *neededTypes = new std::set<std::string>();

  // unique_ptr can't be copied, so the vector must be moved into MultiplexConsumer.
  // Building a named local makes that std::move explicit and unambiguous.
  std::vector<std::unique_ptr<clang::ASTConsumer>> consumers;
  consumers.emplace_back(std::make_unique<CountingConsumer>(_Types, toFilter));
  consumers.emplace_back(std::make_unique<FilterFunctionsConsumer>(toFilter, toRemove, _Config));
  consumers.emplace_back(std::make_unique<RemoveConsumer>(_Rewriter, toRemove, neededTypes));
  consumers.emplace_back(
      std::make_unique<AddVerifiersConsumerFilter>(_Output, neededTypes, _Rewriter));

  return std::make_unique<clang::MultiplexConsumer>(std::move(consumers));
}

bool FilterAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  return clang::ASTFrontendAction::BeginSourceFileAction(compiler);
}

void FilterAction::EndSourceFileAction() {
  _Rewriter.getEditBuffer(getCompilerInstance().getSourceManager().getMainFileID()).write(_Output);
}
