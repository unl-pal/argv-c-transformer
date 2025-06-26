#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <vector>

class GenerateIncludeHandler : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  /// Matcher that finds and handles the include statements needed then prints to output
  GenerateIncludeHandler(clang::SourceManager &mgr, llvm::raw_fd_ostream &output);

  /// runs the matcher and provides a way to interract with the results
  void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);

  /// getter for all Includes
  std::vector<std::string> getAllI();

private:
	std::vector<std::string> _AllInc;
	clang::SourceManager &_Mgr;
	llvm::raw_fd_ostream &_Output; // how does transformer do this?
	// clang::ast_matchers::MatchFinder::MatchResult &Result;
};
