# pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>

class RemoveFuncVisitor : public clang::RecursiveASTVisitor<RemoveFuncVisitor> {
public:
  RemoveFuncVisitor(clang::ASTContext *C, clang::Rewriter &rewriter, std::vector<std::string> toRemove);

  bool VisitFunctionDecl(clang::FunctionDecl *D);

  bool VisitCallExpr(clang::CallExpr *E);

  bool shouldTraversePostOrder();

private:
  clang::ASTContext *_C;
  clang::SourceManager &_mgr;
  clang::Rewriter &_Rewriter;
  std::vector<std::string> _toRemove;
};
