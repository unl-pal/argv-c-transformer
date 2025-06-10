#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <ostream>
#include <vector>

class HandlerFindInclude : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  HandlerFindInclude(clang::SourceManager &mgr,
										 std::ostream &output)
			: _AllInc(),
				_Mgr(mgr),
				_Output(output) {}

  void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);

	std::vector<std::string> getAllI();

private:
	std::vector<std::string> _AllInc;
	clang::SourceManager &_Mgr;
	std::ostream &_Output; // how does transformer do this?
	// clang::ast_matchers::MatchFinder::MatchResult &Result;
};
