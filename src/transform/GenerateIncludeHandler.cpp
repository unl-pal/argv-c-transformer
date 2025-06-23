#include "include/GenerateIncludeHandler.hpp"

#include <algorithm>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/PreprocessingRecord.h>
#include <filesystem>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

/*
 * THIS CODE MAY NO LONGER BE NEEDED AT ALL BUT IS BEING KEPT FOR REFERENCE
 * MATERIAL UNTIL OTHER EXAMPLES ARE CREATED
 */

GenerateIncludeHandler::GenerateIncludeHandler(clang::SourceManager &mgr, llvm::raw_fd_ostream &output)
			: _AllInc(),
			_Mgr(mgr),
			_Output(output) {}

void
GenerateIncludeHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  if (const clang::Decl *decl =
    Result.Nodes.getNodeAs<clang::Decl>("includes")) {
    if (decl->isReferenced() || decl->isUsed()) {
      std::string fileName =
	_Mgr.getFilename(decl->getLocation())
	.rsplit(std::filesystem::path::preferred_separator)
	.second.str();
      if (std::find(_AllInc.begin(), _AllInc.end(), fileName) == _AllInc.end()) {
	_AllInc.push_back(fileName);
      }
    }
  }
}

std::vector<std::string> GenerateIncludeHandler::getAllI() {
  return _AllInc;
}
