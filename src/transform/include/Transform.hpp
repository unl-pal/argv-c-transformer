#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>
#include <vector>

class TransformerVisitor : public clang::RecursiveASTVisitor<TransformerVisitor> {
public:
  TransformerVisitor(clang::ASTContext *newC, clang::ASTContext *oldC, clang::Rewriter &R);

  bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *TD);

  bool VisitCallExpr(clang::CallExpr *E);

  bool VisitDeclRefExpr(clang::DeclRefExpr *D);

  bool VisitDecl(clang::Decl *D);

  bool VisitStmt(clang::Stmt *S);

private:
  clang::ASTContext *_NewC;
  clang::ASTContext *_OldC;
  clang::Rewriter &_R;
  clang::SourceManager *_M;
  std::vector<std::string> VerifierFuncs = {
    "__VERIFIER_nondet_char",
    "__VERIFIER_nondet_double",
    "__VERIFIER_nondet_float",
    "__VERIFIER_nondet_int",
    "__VERIFIER_nondet_long",
    "__VERIFIER_nondet_longlong",
    "__VERIFIER_nondet_short",
    "__VERIFIER_nondet_uint"
  };
  std::vector<clang::CanQualType> ReturnTypes = {
  _NewC->CharTy,
  _NewC->DoubleTy,
  _NewC->FloatTy,
  _NewC->IntTy,
  _NewC->LongTy,
  _NewC->LongLongTy,
  _NewC->ShortTy,
  _NewC->UnsignedIntTy
  };
};
