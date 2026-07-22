// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <set>
#include <string>

/**
 * @brief PPCallbacks hook that strips non-system #include directives.
 *
 * System headers (C stdlib and platform headers) are kept; project-local
 * includes are removed from the output, since every function they declare is
 * havocked by HavocCallsConsumer anyway. A file that uses types or macros
 * from a local header will no longer compile after stripping — those outputs
 * are weeded out by keepCompilesOnly (header type/macro handling is a future
 * feature).
 */
class IncludeFinder : public clang::PPCallbacks {
public:
  /**
   * @brief Constructs the callback, binding the source manager and rewriter.
   *
   * @param SM       Source manager, used to check whether directives are in the main file.
   * @param rewriter Shared rewriter the include directives are removed through.
   */
  IncludeFinder(clang::SourceManager &SM, clang::Rewriter &rewriter,
                std::shared_ptr<std::set<std::string>> existingIncludes);

  /**
   * @brief Called by the preprocessor for each #include/#import directive.
   *
   * @param HashLoc       Location of the '#' that begins the directive.
   * @param FileName      Name of the included file as written in the source.
   * @param IsAngled      True for <...> includes, false for "..." includes.
   * @param FilenameRange Source range of the filename text.
   * @param FileType      Characteristic kind (system/user) of the included file.
   */
  void InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &IncludeTok,
                          llvm::StringRef FileName, bool IsAngled,
                          clang::CharSourceRange FilenameRange, clang::OptionalFileEntryRef File,
                          llvm::StringRef SearchPath, llvm::StringRef RelativePath,
                          const clang::Module *SuggestedModule, bool ModuleImported,
                          clang::SrcMgr::CharacteristicKind FileType) override;

private:
  clang::SourceManager &_Mgr;
  clang::Rewriter &_Rewriter;
  std::shared_ptr<std::set<std::string>> _ExistingIncludes;
};

/**
 * @brief ASTFrontendAction that drives the transform consumer pipeline.
 *
 * Owns the Rewriter shared by all consumers, registers IncludeFinder on the
 * preprocessor, and writes the rewritten source to output once all consumers
 * have run.
 */
class TransformAction : public clang::ASTFrontendAction {
public:
  /**
   * @brief Constructs the action, binding the output stream.
   *
   * @param output Stream the transformed source is written to (a file in
   *               production, a string stream in tests).
   */
  TransformAction(llvm::raw_ostream &output);

  /**
   * @brief Builds the multiplexed consumer chain for the transform pipeline.
   *
   * Registers IncludeFinder on the preprocessor and returns a
   * MultiplexConsumer running HavocCallsConsumer, MainGenConsumer, and
   * AddVerifiersConsumer (in that order) over the shared Rewriter.
   *
   * @param Compiler The compiler instance for this translation unit.
   * @param Filename Name of the file being processed.
   * @return The multiplexed consumer chain.
   */
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler,
                                                                llvm::StringRef Filename) override;

  /**
   * @brief Initializes the shared Rewriter before any consumers run.
   *
   * @param compiler The compiler instance for this translation unit.
   * @return Result of the base class implementation.
   */
  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

  /**
   * @brief Writes the Rewriter's edited buffer to output after all consumers have run.
   */
  void EndSourceFileAction() override;

private:
  llvm::raw_ostream &_Output;
  clang::Rewriter _Rewriter;
};

/**
 * @brief Carries the output stream into Clang's tool runner.
 *
 * Clang's {@code ClangTool::run()} only knows how to call {@code create()} on
 * a {@code FrontendActionFactory}. This subclass stores the output stream so
 * that each {@code TransformAction} it creates can write the rewritten source
 * without that stream being a global.
 */
class ArgsFrontendFactory : public clang::tooling::FrontendActionFactory {
public:
  /**
   * @brief Constructs the factory, binding the output stream.
   *
   * @param output Reference to the output stream for the transformed file.
   */
  ArgsFrontendFactory(llvm::raw_ostream &output);

  /**
   * @brief Called by {@code ClangTool} once per source file to create the action.
   *
   * Returns a new {@code TransformAction} loaded with the output stream.
   *
   * @return Owning pointer to the created action.
   */
  std::unique_ptr<clang::FrontendAction> create() override;

private:
  llvm::raw_ostream &_Output;
};
