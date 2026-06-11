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
  IncludeFinder(clang::SourceManager &SM, clang::Rewriter &rewriter);

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
  clang::Rewriter &_Rewriter;
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
  llvm::raw_fd_ostream &_Output;
  clang::Rewriter _Rewriter;
};
