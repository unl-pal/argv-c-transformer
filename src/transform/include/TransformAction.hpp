#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <set>

/**
 * @brief PPCallbacks hook that observes #include/#import directives during preprocessing.
 *
 * Currently only tracks which standard headers have been seen so far in the
 * main file. Intended to drive include regeneration and, eventually,
 * classification of non-standard includes as internal (same-repo) vs
 * external dependencies.
 */
class IncludeFinder : public clang::PPCallbacks {
public:
  /**
   * @brief Constructs the callback, binding the source manager and output stream.
   *
   * @param SM     Source manager, used to check whether directives are in the main file.
   * @param output Stream that regenerated #include statements are written to.
   */
  IncludeFinder(clang::SourceManager &SM, llvm::raw_fd_ostream &output);

  /**
   * @brief Called by the preprocessor for each #include/#import directive.
   *
   * @param HashLoc         Location of the '#' that begins the directive.
   * @param IncludeTok      The "include"/"import" token.
   * @param FileName        Name of the included file as written in the source.
   * @param IsAngled        True for <...> includes, false for "..." includes.
   * @param FilenameRange   Source range of the filename text.
   * @param File            The resolved file, if Clang found it.
   * @param SearchPath      Directory in which the file was found.
   * @param RelativePath    Path of the file relative to SearchPath.
   * @param SuggestedModule Module suggested for the include, if any.
   * @param ModuleImported  Whether the include was treated as a module import.
   * @param FileType        Characteristic kind (system/user) of the included file.
   */
  void InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &IncludeTok,
                          llvm::StringRef FileName, bool IsAngled,
                          clang::CharSourceRange FilenameRange, clang::OptionalFileEntryRef File,
                          llvm::StringRef SearchPath, llvm::StringRef RelativePath,
                          const clang::Module *SuggestedModule, bool ModuleImported,
                          clang::SrcMgr::CharacteristicKind FileType) override;

private:
  clang::SourceManager &_Mgr;
  llvm::raw_fd_ostream &_Output;
  std::set<llvm::StringRef> _AlreadyIncluded;
  std::set<llvm::StringRef> _AllStandardHeaders = {
      "assert.h",      "complex.h",  "ctype.h",  "errno.h",     "fenv.h",   "float.h",
      "inttypes.h",    "iso646.h",   "limits.h", "locale.h",    "math.h",   "setjmp.h",
      "signal.h",      "stdalign.h", "stdarg.h", "stdatomic.h", "stdbit.h", "stdbool.h",
      "stdckdint.h",   "stddef.h",   "stdint.h", "stdio.h",     "stdlib.h", "stdmchar.h",
      "stdnoreturn.h", "string.h",   "tgmath.h", "threads.h",   "time.h",   "uchar.h",
      "wchar.h",       "wctype.h"};
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
   * @param output Stream the transformed source is written to.
   */
  TransformAction(llvm::raw_fd_ostream &output);

  /**
   * @brief Builds the multiplexed consumer chain for the transform pipeline.
   *
   * Registers IncludeFinder on the preprocessor and returns a
   * MultiplexConsumer running ReplaceDeadCallsConsumer,
   * AddVerifiersConsumer, and IsThereMainConsumer (in that order) over the
   * shared Rewriter.
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
  llvm::raw_fd_ostream &_Output;
  clang::Rewriter _Rewriter;
};
