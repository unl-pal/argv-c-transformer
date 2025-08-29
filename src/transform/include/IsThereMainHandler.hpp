#pragma once

#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <set>

class IsThereMainHandler : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  // IsThereMainHandler(clang::Rewriter &rewriter);
  // IsThereMainHandler(std::set<clang::CallExpr*> &callsToMake);
  IsThereMainHandler(std::set<clang::DeclRefExpr*> &callsToMake);

  void run(const clang::ast_matchers::MatchFinder::MatchResult &results);

  bool HasMain();

private:
  bool _hasMain;
  // clang::Rewriter &_Rewriter;
  // std::set<clang::CallExpr*> &_CallsToMake;
  std::set<clang::DeclRefExpr*> &_CallsToMake;
};
