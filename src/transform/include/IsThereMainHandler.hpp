#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>

class IsThereMainHandler : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  IsThereMainHandler();

  void run(const clang::ast_matchers::MatchFinder::MatchResult &results);

  bool HasMain();

private:
  bool _hasMain;
};
