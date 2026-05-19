#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <string>
#include <vector>

class AllFunctionsToCallHandler : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  /// Handle for finding all functions that need a function call added to the
  /// code to make reachable
  AllFunctionsToCallHandler();

  /// Runs the matcher on the AST
  /// @param results is the reference to all found functions needdig a function
  /// call added to main
  void run(const clang::ast_matchers::MatchFinder::MatchResult &results);

  /// Retrieves the names of all functions found needing a function call added
  /// to main
  std::vector<std::string> GetNames();

private:
  std::vector<std::string> _Names;
};
