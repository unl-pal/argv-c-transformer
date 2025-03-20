#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Rewrite/Core/Rewriter.h>

class IfRewriteVisitor : public clang::RecursiveASTVisitor<IfRewriteVisitor> {
public:
IfRewriteVisitor(clang::Rewriter &R, clang::ASTContext *C);

// May not have needed to create this
bool VisitDecl(clang::Decl *D);

// May not have needed to create this
bool VisitType(clang::Type *T);

bool VisitStmt(clang::Stmt *S);

bool VisitIfStmt(clang::IfStmt *I);

private:
  clang::Rewriter &_Rewrite;
  clang::ASTContext *_Context;
};
