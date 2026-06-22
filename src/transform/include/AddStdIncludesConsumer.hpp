#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

class AddStdIncludesConsumer : public clang::ASTConsumer {
public:
  AddStdIncludesConsumer(std::shared_ptr<std::set<std::string>> existingIncludes,
                         clang::Rewriter &rewriter);
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _ExistingIncludes;
  clang::Rewriter &_Rewriter;
};
