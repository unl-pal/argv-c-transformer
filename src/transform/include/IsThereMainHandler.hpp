#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Rewrite/Core/Rewriter.h>

class IsThereMainHandler : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  IsThereMainHandler(clang::Rewriter &rewriter);

  void run(const clang::ast_matchers::MatchFinder::MatchResult &results);

  bool HasMain();

private:
  bool _hasMain;
  clang::Rewriter &_Rewriter;
};
