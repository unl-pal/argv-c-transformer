#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>

class TransformerVisitor : public clang::RecursiveASTVisitor<TransformerVisitor> {
public:
  TransformerVisitor(clang::ASTContext *C, clang::Rewriter &R);

  bool VisitCallExpr(clang::CallExpr *E);

  bool VisitDeclRefExpr(clang::DeclRefExpr *D);

  bool VisitDecl(clang::Decl *D);

  bool VisitStmt(clang::Stmt *S);

private:
  clang::ASTContext *_C;
  clang::Rewriter &_R;
  clang::SourceManager *_M;
};
