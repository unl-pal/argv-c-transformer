#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>

class CreateNewAST {
public:

  CreateNewAST(clang::Rewriter &R, clang::SourceManagerForFile &SMF);

  bool AddVerifiers(clang::ASTContext *newC, clang::ASTContext *oldC);

  bool AddBoolDef(clang::ASTContext *newC, clang::ASTContext *oldC);

  bool AddAllDecl(clang::ASTContext *newC, clang::ASTContext *oldC);

private:
  clang::Rewriter _R;
  clang::SourceManagerForFile &_SMF;
};
