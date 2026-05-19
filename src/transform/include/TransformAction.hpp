#pragma once

#include <GenerateIncludeConsumer.hpp>
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

class IncludeFinder : public clang::PPCallbacks {
public:

  // Constructor to get the SourceManager and output stream to print to
  IncludeFinder(clang::SourceManager &SM, llvm::raw_fd_ostream &output);

  /// Finds and prints the Inclusion statements needed for the code
  /// Due to being overridded the parameters are needed but are not implemented
  void InclusionDirective(clang::SourceLocation HashLoc,
        const clang::Token & IncludeTok,
        llvm::StringRef FileName,
        bool IsAngled,
        clang::CharSourceRange FilenameRange,
        clang::OptionalFileEntryRef File,
        llvm::StringRef SearchPath,
        llvm::StringRef RelativePath,
        const clang::Module * SuggestedModule,
        bool ModuleImported,
        clang::SrcMgr::CharacteristicKind FileType) override;

private:
  clang::SourceManager &_Mgr;
  llvm::raw_fd_ostream &_Output;
  std::set<llvm::StringRef> _AlreadyIncluded;
  std::set<llvm::StringRef> _AllStandardHeaders = {
    "assert.h",    "complex.h",  "ctype.h",   "errno.h",     "fenv.h",
    "float.h",     "inttypes.h", "iso646.h",  "limits.h",    "locale.h",
    "math.h",      "setjmp.h",   "signal.h",  "stdalign.h",  "stdarg.h",
    "stdatomic.h", "stdbit.h",   "stdbool.h", "stdckdint.h", "stddef.h",
    "stdint.h",    "stdio.h",    "stdlib.h",  "stdmchar.h",  "stdnoreturn.h",
    "string.h",    "tgmath.h",   "threads.h", "time.h",      "uchar.h",
    "wchar.h",     "wctype.h"};
};

class TransformAction : public clang::ASTFrontendAction {
public:
  /// Constructor for the Action that handles the transformation of code and
  /// creates all consumers that will be used for the task
  TransformAction(llvm::raw_fd_ostream &output);

  /// Returns a multiplexor instead to allow many consumers to be created and called
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef          Filename) override;

  /// Runs after the compiler but before the creation of consumers and running of visitors
  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

  /// Runs after all consumers but before the AST is destroyed
  void EndSourceFileAction() override;

  // bool usesPreprocessorOnly() const override;

private:
  llvm::raw_fd_ostream &_Output;
  clang::Rewriter _Rewriter;
};

