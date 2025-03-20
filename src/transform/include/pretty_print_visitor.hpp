#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>

class PrintASTVisitor : public clang::RecursiveASTVisitor<PrintASTVisitor> {
public:
PrintASTVisitor(clang::ASTContext *Context);

bool VisitNamedDecl(clang::NamedDecl *D);

bool TraverseDecl(clang::Decl *D);

bool VisitCXXRecordDecl(clang::CXXRecordDecl *Declaration);

private:
  int recursionDepth;
  clang::ASTContext *_Context;

};
