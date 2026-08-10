// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "TransformAction.hpp"
#include "AddStdIncludesConsumer.hpp"
#include "AddVerifiersConsumer.hpp"
#include "DebugLog.hpp"
#include "HavocCallsConsumer.hpp"
#include "MainGenConsumer.hpp"
#include "UnknownTypeDiagConsumer.hpp"

#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/MacroArgs.h>
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

// assert(cond) always expands with the macro name token first and the
// closing paren as the invocation's last token, so Range brackets exactly
// the text to replace
void AssertRewriter::MacroExpands(const clang::Token &MacroNameTok, const clang::MacroDefinition &MD,
                                  clang::SourceRange Range, const clang::MacroArgs *) {
  const clang::IdentifierInfo *id = MacroNameTok.getIdentifierInfo();
  if (!id || id->getName() != "assert")
    return;
  const clang::MacroInfo *info = MD.getMacroInfo();
  if (!info || !info->isFunctionLike())
    return;
  if (!Range.getBegin().isFileID() || !_Mgr.isInMainFile(Range.getBegin()))
    return;

  clang::CharSourceRange charRange = clang::CharSourceRange::getTokenRange(Range);
  bool invalid = false;
  llvm::StringRef text = clang::Lexer::getSourceText(charRange, _Mgr, _LangOpts, &invalid);
  if (invalid)
    return;

  size_t openParen = text.find('(');
  size_t closeParen = text.rfind(')');
  if (openParen == llvm::StringRef::npos || closeParen == llvm::StringRef::npos ||
      closeParen <= openParen)
    return;
  llvm::StringRef cond = text.substr(openParen + 1, closeParen - openParen - 1);

  debugLog(3, "[transform] rewrote assert(" + cond.str() + ") -> reach_error()");
  _Rewriter.ReplaceText(charRange, ("if (!(" + cond + ")) reach_error()").str());
  _NeededSuffixes->insert("__reach_error");
}

AssertRewriter::AssertRewriter(clang::SourceManager &SM, clang::Rewriter &rewriter,
                               std::shared_ptr<std::set<std::string>> neededSuffixes,
                               const clang::LangOptions &langOpts)
    : _Mgr(SM), _Rewriter(rewriter), _NeededSuffixes(neededSuffixes), _LangOpts(langOpts) {}

TransformAction::TransformAction(llvm::raw_ostream &output)
    : _Output(output), _Rewriter(),
      _UnresolvedTypeNames(std::make_shared<std::set<std::string>>()) {}

// unique_ptr can't be copied, so tempVector is built up and moved into MultiplexConsumer.
std::unique_ptr<clang::ASTConsumer>
TransformAction::CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef) {
  auto existingIncludes = std::make_shared<std::set<std::string>>();
  // Verifier suffixes needed by call havocking and the generated main;
  // AddVerifiersConsumer runs last and emits the extern declarations
  auto neededSuffixes = std::make_shared<std::set<std::string>>();

  clang::Preprocessor &pp = compiler.getPreprocessor();
  pp.addPPCallbacks(
      std::make_unique<IncludeFinder>(compiler.getSourceManager(), _Rewriter, existingIncludes));
  pp.addPPCallbacks(std::make_unique<AssertRewriter>(compiler.getSourceManager(), _Rewriter,
                                                     neededSuffixes, pp.getLangOpts()));

  // Functions HavocCallsConsumer found to have collapsed entirely to no-ops;
  // MainGenConsumer skips harnessing them
  auto noOpFunctions = std::make_shared<std::set<std::string>>();

  std::vector<std::unique_ptr<clang::ASTConsumer>> tempVector;
  tempVector.emplace_back(
      std::make_unique<HavocCallsConsumer>(neededSuffixes, noOpFunctions, _Rewriter));
  tempVector.emplace_back(
      std::make_unique<MainGenConsumer>(neededSuffixes, noOpFunctions, _Rewriter));
  tempVector.emplace_back(std::make_unique<AddVerifiersConsumer>(neededSuffixes, _Rewriter));
  tempVector.emplace_back(
      std::make_unique<AddStdIncludesConsumer>(existingIncludes, _UnresolvedTypeNames, _Rewriter));

  return std::make_unique<clang::MultiplexConsumer>(std::move(tempVector));
}

bool TransformAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  // Ownership passes to the DiagnosticsEngine; it outlives parsing, which is
  // all this consumer needs to run for.
  compiler.getDiagnostics().setClient(new UnknownTypeDiagConsumer(_UnresolvedTypeNames),
                                      /*ShouldOwnClient=*/true);
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
