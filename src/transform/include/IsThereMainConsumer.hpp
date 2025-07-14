#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>

class IsThereMainConsumer : public clang::ASTConsumer {
public:
  IsThereMainConsumer(clang::Rewriter &rewriter);

  virtual void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  clang::Rewriter &_Rewriter;
};
