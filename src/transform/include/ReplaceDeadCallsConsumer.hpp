#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Type.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/ADT/StringRef.h>
#include <set>

class ReplaceDeadCallsConsumer : public clang::ASTConsumer {
public:
  /// Consumer used to replace calls for functions that had been previously removed
  ReplaceDeadCallsConsumer(std::set<clang::QualType> *neededTypes, clang::Rewriter &rewriter);

  /// Launches the REplaceDeadCallsVisitor and fills the neededTypes set
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::set<clang::QualType> *_NeededTypes;
  clang::Rewriter &_Rewriter;
};
