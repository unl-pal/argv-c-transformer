#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

class AddVerifiersConsumer : public clang::ASTConsumer {
public:
  /// Inserts an extern __VERIFIER_nondet_<suffix>(void) declaration at the
  /// top of the main file for every needed suffix not already declared
  /// (the filter step may have injected some already).
  /// @param neededSuffixes verifier suffixes collected by earlier consumers
  /// @param rewriter rewriter for changing source code of AST
  AddVerifiersConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                       clang::Rewriter &rewriter);

  /// Inserts the extern declarations
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
};
