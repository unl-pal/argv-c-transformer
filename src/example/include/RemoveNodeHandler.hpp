#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>

class RemoveNodeHandler : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  // using MatchResult = clang::ast_matchers::MatchFinder::MatchResult;
  void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);

  void onEndOfTranslationUnit();
};
