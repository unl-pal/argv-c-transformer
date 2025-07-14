#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>
#include <vector>
class AddMainVisitor : public clang::RecursiveASTVisitor<AddMainVisitor> {
public:
  AddMainVisitor(clang::ASTContext *c, std::vector<std::string> functionsToCall,
                 clang::Rewriter &r);

  bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *D);

private:
  clang::ASTContext *_Context;
  std::vector<std::string> &_FunctionsToCall;
  clang::Rewriter &_Rewriter;

};
