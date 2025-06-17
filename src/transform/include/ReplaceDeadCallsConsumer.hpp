#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Type.h>
#include <llvm/ADT/StringRef.h>
#include <set>

class ReplaceDeadCallsConsumer : public clang::ASTConsumer {
public:
  ReplaceDeadCallsConsumer(std::set<clang::QualType> *neededTypes);

  virtual void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::set<clang::QualType> *_NeededTypes;
};
