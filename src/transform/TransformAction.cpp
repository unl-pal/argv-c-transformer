// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "TransformAction.hpp"
#include "AddStdIncludesConsumer.hpp"
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

// assert(cond) expands with the macro name token first and the closing paren
// last, so Range brackets exactly the text to replace.
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

  auto openParen = text.find('(');
  auto closeParen = text.rfind(')');
  if (openParen == llvm::StringRef::npos || closeParen == llvm::StringRef::npos ||
      closeParen <= openParen)
    return;
  llvm::StringRef cond = text.substr(openParen + 1, closeParen - openParen - 1);

  debugLog(3, "[transform] rewrote assert(" + cond.str() + ") -> reach_error()");
  _Rewriter.ReplaceText(charRange, ("if (!(" + cond + ")) reach_error()").str());
}

AssertRewriter::AssertRewriter(clang::SourceManager &SM, clang::Rewriter &rewriter,
                               const clang::LangOptions &langOpts)
    : _Mgr(SM), _Rewriter(rewriter), _LangOpts(langOpts) {}

TransformAction::TransformAction(llvm::raw_ostream &output, const HavocBounds &havoc)
    : _Output(output), _Rewriter(),
      _UnresolvedTypeNames(std::make_shared<std::set<std::string>>()), _Havoc(havoc) {}

std::unique_ptr<clang::ASTConsumer>
TransformAction::CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef) {
  auto existingIncludes = std::make_shared<std::set<std::string>>();

  clang::Preprocessor &pp = compiler.getPreprocessor();
  pp.addPPCallbacks(
      std::make_unique<IncludeFinder>(compiler.getSourceManager(), _Rewriter, existingIncludes));
  pp.addPPCallbacks(
      std::make_unique<AssertRewriter>(compiler.getSourceManager(), _Rewriter, pp.getLangOpts()));

  // HavocCallsConsumer fills this; MainGenConsumer skips harnessing them.
  auto noOpFunctions = std::make_shared<std::set<std::string>>();

  std::vector<std::unique_ptr<clang::ASTConsumer>> tempVector;
  tempVector.emplace_back(std::make_unique<HavocCallsConsumer>(noOpFunctions, _Rewriter));
  tempVector.emplace_back(std::make_unique<MainGenConsumer>(noOpFunctions, _Rewriter, _Havoc));
  tempVector.emplace_back(
      std::make_unique<AddStdIncludesConsumer>(existingIncludes, _UnresolvedTypeNames, _Rewriter));

  return std::make_unique<clang::MultiplexConsumer>(std::move(tempVector));
}

bool TransformAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  // Ownership passes to the DiagnosticsEngine, which outlives parsing.
  compiler.getDiagnostics().setClient(new UnknownTypeDiagConsumer(_UnresolvedTypeNames),
                                      /*ShouldOwnClient=*/true);
  return clang::ASTFrontendAction::BeginSourceFileAction(compiler);
}

void TransformAction::EndSourceFileAction() {
  _Rewriter.getEditBuffer(getCompilerInstance().getSourceManager().getMainFileID()).write(_Output);
}

ArgsFrontendFactory::ArgsFrontendFactory(llvm::raw_ostream &output, const HavocBounds &havoc)
    : _Output(output), _Havoc(havoc) {}

std::unique_ptr<clang::FrontendAction> ArgsFrontendFactory::create() {
  return std::make_unique<TransformAction>(_Output, _Havoc);
}
