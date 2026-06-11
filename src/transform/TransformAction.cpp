#include "TransformAction.hpp"
#include "AddVerifiersConsumer.hpp"
#include "HavocCallsConsumer.hpp"
#include "MainGenConsumer.hpp"

#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
#include <memory>
#include <vector>

// Strip non-system includes: their functions are havocked by
// HavocCallsConsumer, so the directives only leave unresolvable references
// in the output. Unresolved includes also classify as C_User and get
// dropped. Files that needed a local header's types or macros stop
// compiling and are weeded out by keepCompilesOnly.
void IncludeFinder::InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &,
                                       llvm::StringRef, bool,
                                       clang::CharSourceRange FilenameRange,
                                       clang::OptionalFileEntryRef, llvm::StringRef,
                                       llvm::StringRef, const clang::Module *, bool,
                                       clang::SrcMgr::CharacteristicKind FileType) {
  if (!_Mgr.isInMainFile(HashLoc) || FileType != clang::SrcMgr::C_User)
    return;
  _Rewriter.RemoveText(clang::CharSourceRange::getCharRange(HashLoc, FilenameRange.getEnd()));
}

IncludeFinder::IncludeFinder(clang::SourceManager &SM, clang::Rewriter &rewriter)
    : _Mgr(SM), _Rewriter(rewriter) {}

TransformAction::TransformAction(llvm::raw_fd_ostream &output) : _Output(output), _Rewriter() {}

// Builds the multiplexed consumer chain; tempVector is built up and moved into
// the MultiplexConsumer to avoid type-inference/optimization issues seen previously
std::unique_ptr<clang::ASTConsumer>
TransformAction::CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef) {
  clang::Preprocessor &pp = compiler.getPreprocessor();
  pp.addPPCallbacks(std::make_unique<IncludeFinder>(compiler.getSourceManager(), _Rewriter));

  // Verifier suffixes needed by call havocking and the generated main;
  // AddVerifiersConsumer runs last and emits the extern declarations
  auto neededSuffixes = std::make_shared<std::set<std::string>>();

  std::vector<std::unique_ptr<clang::ASTConsumer>> tempVector;
  tempVector.emplace_back(std::make_unique<HavocCallsConsumer>(neededSuffixes, _Rewriter));
  tempVector.emplace_back(std::make_unique<MainGenConsumer>(neededSuffixes, _Rewriter));
  tempVector.emplace_back(std::make_unique<AddVerifiersConsumer>(neededSuffixes, _Rewriter));

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
