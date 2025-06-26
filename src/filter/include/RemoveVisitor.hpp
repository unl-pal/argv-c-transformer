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
  /// Constructs a Visitor that removes functions specified in toRemove using
  /// the rewriter
  RemoveFuncVisitor(clang::ASTContext *C, clang::Rewriter &rewriter, std::vector<std::string> toRemove);

  /// Visits all function declarations checking the name agains the functions to
  /// remove and handles the function declaration and any associated comments
  bool VisitFunctionDecl(clang::FunctionDecl *D);

  /// Currently not in use but could be implemented to handle Calls to the
  /// deleted functions
  bool VisitCallExpr(clang::CallExpr *E);

  /// Tells the RecursiveASTVisitor wether to recurse depth or breadth first
  bool shouldTraversePostOrder();

private:
  clang::ASTContext *_C;
  clang::SourceManager &_mgr;
  clang::Rewriter &_Rewriter;
  std::vector<std::string> _toRemove;
};
