#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

class MainGenConsumer : public clang::ASTConsumer {
public:
  /// Generates the benchmark entry point. Any pre-existing main (including
  /// its forward declarations) is renamed to original_main, then a fresh
  /// main(void) is appended that calls every function defined in the file
  /// with __VERIFIER_nondet_* arguments. Functions with a parameter type
  /// that has no nondet equivalent (pointers, structs, ...) and variadic
  /// functions are skipped.
  /// @param neededSuffixes verifier suffixes used by the harness, shared
  ///        with AddVerifiersConsumer which emits the extern declarations
  /// @param rewriter rewriter for changing source code of AST
  MainGenConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                  clang::Rewriter &rewriter);

  /// Renames an existing main and appends the generated harness main
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
};
