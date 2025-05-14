#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
class ReplaceDeadCallsVisitor : public clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor> {
public:
  ReplaceDeadCallsVisitor(clang::ASTContext *C, clang::Rewriter &R);
  bool VisitDecl(clang::Decl *D);
  bool VisitCallExpr(clang::CallExpr *E);

private:
  clang::ASTContext *_C;
  clang::Rewriter &_R;
};
