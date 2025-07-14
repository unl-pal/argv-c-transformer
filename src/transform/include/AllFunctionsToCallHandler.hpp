#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <string>
#include <vector>

class AllFunctionsToCallHandler : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  AllFunctionsToCallHandler();

  void run(const clang::ast_matchers::MatchFinder::MatchResult &results);

  std::vector<std::string> GetNames();

private:
  std::vector<std::string> _Names;
};
