#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <set>

class ReplaceDeadCallsVisitor : public clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor> {
public:
  ReplaceDeadCallsVisitor(clang::ASTContext *C, std::set<clang::QualType> *neededTypes);

  virtual bool VisitTranslationUnit(clang::TranslationUnitDecl *D);

  virtual bool VisitDecl(clang::Decl *D);

  virtual bool VisitCallExpr(clang::CallExpr *E);

  bool shouldTraversePostOrder();

private:
  clang::ASTContext *_C;
  std::set<clang::QualType> *_NeededTypes;
};
