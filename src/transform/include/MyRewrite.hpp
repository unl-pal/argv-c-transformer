#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Rewrite/Core/Rewriter.h>


class MyRewriteMain : clang::RecursiveASTVisitor<MyRewriteMain> {
  MyRewriteMain(clang::Rewriter &R);

public:
  void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);

private:
  clang::Rewriter _Rewriter;
};
