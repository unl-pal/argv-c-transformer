#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <string>
#include <vector>
class AddMainVisitor : public clang::RecursiveASTVisitor<AddMainVisitor> {
public:
  AddMainVisitor(std::vector<std::string> functionsToCall);

bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *D);

private:
  std::vector<std::string> &_FunctionsToCall;

};
