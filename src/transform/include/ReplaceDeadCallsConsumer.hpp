#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

class ReplaceDeadCallsConsumer : public clang::ASTConsumer {
public:
  /// Consumer used to replace calls for functions that had been previously removed
  ReplaceDeadCallsConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                           clang::Rewriter &rewriter);

  /// Launches the ReplaceDeadCallsVisitor and fills the neededSuffixes set
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
};
