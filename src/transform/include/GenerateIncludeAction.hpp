#pragma once

#include <GenerateIncludeConsumer.hpp>
#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <set>

class IncludeFinder : public clang::PPCallbacks {
public:

  // Constructor to get the SourceManager
  IncludeFinder(clang::SourceManager &SM, llvm::raw_fd_ostream &output);

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

class GenerateIncludeAction : public clang::ASTFrontendAction {
public:
  GenerateIncludeAction(llvm::raw_fd_ostream &output);

  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef          Filename) override;

  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

  void EndSourceFileAction() override;

  // bool usesPreprocessorOnly() const override;

private:
  llvm::raw_fd_ostream &_Output;
};

