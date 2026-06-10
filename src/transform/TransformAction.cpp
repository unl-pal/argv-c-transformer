#include "TransformAction.hpp"
#include "AddVerifiersConsumer.hpp"
#include "ReplaceDeadCallsConsumer.hpp"

#include <IsThereMainConsumer.hpp>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
#include <memory>
#include <vector>

// Tracks standard headers seen so far; output/regeneration logic is not yet implemented
void IncludeFinder::InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &,
                                       llvm::StringRef FileName, bool, clang::CharSourceRange,
                                       clang::OptionalFileEntryRef, llvm::StringRef,
                                       llvm::StringRef, const clang::Module *, bool,
                                       clang::SrcMgr::CharacteristicKind) {
  if (_Mgr.isInMainFile(HashLoc)) {
    if (_AllStandardHeaders.count(FileName)) {
      _AlreadyIncluded.emplace(FileName);
    }
  }
}

IncludeFinder::IncludeFinder(clang::SourceManager &SM, llvm::raw_fd_ostream &output)
    : _Mgr(SM), _Output(output) {}

TransformAction::TransformAction(llvm::raw_fd_ostream &output) : _Output(output), _Rewriter() {}

// Builds the multiplexed consumer chain; tempVector is built up and moved into
// the MultiplexConsumer to avoid type-inference/optimization issues seen previously
std::unique_ptr<clang::ASTConsumer>
TransformAction::CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef) {
  clang::Preprocessor &pp = compiler.getPreprocessor();
  pp.addPPCallbacks(std::make_unique<IncludeFinder>(compiler.getSourceManager(), this->_Output));

  // Shared between ReplaceDeadCallsConsumer and AddVerifiersConsumer
  auto neededTypes = std::make_shared<std::set<clang::QualType>>();

  std::vector<std::unique_ptr<clang::ASTConsumer>> tempVector;
  tempVector.emplace_back(std::make_unique<ReplaceDeadCallsConsumer>(neededTypes, _Rewriter));
  tempVector.emplace_back(std::make_unique<AddVerifiersConsumer>(_Output, neededTypes, _Rewriter));
  tempVector.emplace_back(std::make_unique<IsThereMainConsumer>(_Rewriter));

  return std::make_unique<clang::MultiplexConsumer>(std::move(tempVector));
}

bool TransformAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  return clang::ASTFrontendAction::BeginSourceFileAction(compiler);
}

void TransformAction::EndSourceFileAction() {
  // Retrieve the edited buffer and write to the new output location
  _Rewriter.getEditBuffer(getCompilerInstance().getSourceManager().getMainFileID()).write(_Output);
}
