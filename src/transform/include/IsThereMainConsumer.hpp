#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>

class IsThereMainConsumer : public clang::ASTConsumer {
public:
  IsThereMainConsumer();

  virtual void HandleTranslationUnit(clang::ASTContext &context) override;

private:
};
