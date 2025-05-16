#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>
#include <vector>

class CreateNewAST {
public:
  // std::unique_ptr<clang::ASTUnit> *_NewU;
  // CreateNewAST(std::unique_ptr<clang::ASTUnit> *oldUnit,
  //              std::unique_ptr<clang::ASTUnit> *newUnit);

  CreateNewAST(clang::Rewriter &R, clang::SourceManagerForFile &SMF);

  bool AddVerifiers(clang::ASTContext *newC, clang::ASTContext *oldC);
  // void AddVerifiers();

  bool AddBoolDef(clang::ASTContext *newC, clang::ASTContext *oldC);

  bool AddAllDecl(clang::ASTContext *newC, clang::ASTContext *oldC);

  // std::unique_ptr<clang::ASTUnit> getAST();

private:
  // std::unique_ptr<clang::ASTUnit> *_OldU;
  // clang::ASTContext *_NewC;
  // clang::ASTContext *_OldC;
  clang::Rewriter _R;
  clang::SourceManagerForFile &_SMF;
};
