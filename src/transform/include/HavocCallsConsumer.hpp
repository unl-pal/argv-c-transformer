#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

class HavocCallsConsumer : public clang::ASTConsumer {
public:
  /// Consumer that havocs every function call so each function body is
  /// intraprocedural (see HavocCallsVisitor)
  HavocCallsConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                     clang::Rewriter &rewriter);

  /// Launches the HavocCallsVisitor and fills the neededSuffixes set
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
};
