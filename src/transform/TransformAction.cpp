// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "TransformAction.hpp"
#include "AddStdIncludesConsumer.hpp"
#include "AddVerifiersConsumer.hpp"
#include "DebugLog.hpp"
#include "HavocCallsConsumer.hpp"
#include "MainGenConsumer.hpp"

#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
#include <memory>
#include <vector>

// A quoted include is always project-local by convention and gets stripped
// outright regardless of FileType.
void IncludeFinder::InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &,
                                       llvm::StringRef FileName, bool IsAngled,
                                       clang::CharSourceRange FilenameRange,
                                       clang::OptionalFileEntryRef, llvm::StringRef,
                                       llvm::StringRef, const clang::Module *, bool,
                                       clang::SrcMgr::CharacteristicKind FileType) {
  if (!_Mgr.isInMainFile(HashLoc))
    return;
  if (!IsAngled || FileType == clang::SrcMgr::C_User) {
    debugLog(3, "[transform] stripped project-local include: " + FileName.str());
    _Rewriter.RemoveText(clang::CharSourceRange::getCharRange(HashLoc, FilenameRange.getEnd()));
    return;
  }
  _ExistingIncludes->insert(FileName.str());
}

IncludeFinder::IncludeFinder(clang::SourceManager &SM, clang::Rewriter &rewriter,
                             std::shared_ptr<std::set<std::string>> existingIncludes)
    : _Mgr(SM), _Rewriter(rewriter), _ExistingIncludes(existingIncludes) {}

TransformAction::TransformAction(llvm::raw_ostream &output) : _Output(output), _Rewriter() {}

// unique_ptr can't be copied, so tempVector is built up and moved into MultiplexConsumer.
std::unique_ptr<clang::ASTConsumer>
TransformAction::CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef) {
  auto existingIncludes = std::make_shared<std::set<std::string>>();

  clang::Preprocessor &pp = compiler.getPreprocessor();
  pp.addPPCallbacks(
      std::make_unique<IncludeFinder>(compiler.getSourceManager(), _Rewriter, existingIncludes));

  // Verifier suffixes needed by call havocking and the generated main;
  // AddVerifiersConsumer runs last and emits the extern declarations
  auto neededSuffixes = std::make_shared<std::set<std::string>>();
  // Functions HavocCallsConsumer found to have collapsed entirely to no-ops;
  // MainGenConsumer skips harnessing them
  auto noOpFunctions = std::make_shared<std::set<std::string>>();

  std::vector<std::unique_ptr<clang::ASTConsumer>> tempVector;
  tempVector.emplace_back(
      std::make_unique<HavocCallsConsumer>(neededSuffixes, noOpFunctions, _Rewriter));
  tempVector.emplace_back(
      std::make_unique<MainGenConsumer>(neededSuffixes, noOpFunctions, _Rewriter));
  tempVector.emplace_back(std::make_unique<AddVerifiersConsumer>(neededSuffixes, _Rewriter));
  tempVector.emplace_back(std::make_unique<AddStdIncludesConsumer>(existingIncludes, _Rewriter));

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

ArgsFrontendFactory::ArgsFrontendFactory(llvm::raw_ostream &output) : _Output(output) {}

std::unique_ptr<clang::FrontendAction> ArgsFrontendFactory::create() {
  return std::make_unique<TransformAction>(_Output);
}
