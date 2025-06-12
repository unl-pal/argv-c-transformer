#include "include/GenerateIncludeHandler.hpp"

#include <algorithm>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/PreprocessingRecord.h>
#include <filesystem>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

GenerateIncludeHandler::GenerateIncludeHandler(clang::SourceManager &mgr, llvm::raw_fd_ostream &output)
			: _AllInc(),
			_Mgr(mgr),
			_Output(output) {}

void
GenerateIncludeHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  // llvm::outs() << "inside the run function\n";
  if (const clang::Decl *decl =
    Result.Nodes.getNodeAs<clang::Decl>("includes")) {
    // llvm::outs() << "inside the run function\n";
    if (decl->isReferenced() || decl->isUsed()) {
      // std::string fullname = _Mgr.getFilename(decl->getLocation()).rsplit(std::filesystem::path::preferred_separator).second.str();
      std::string fileName =
	_Mgr.getFilename(decl->getLocation())
	.rsplit(std::filesystem::path::preferred_separator)
	.second.str();
      if (std::find(_AllInc.begin(), _AllInc.end(), fileName) == _AllInc.end()) {
	_AllInc.push_back(fileName);
	// if (std::find(_AllInc.begin(), _AllInc.end(), fileName) == _AllInc.end()) {
	  // _AllInc.push_back(fileName);
	// _Output << "#include <" << fileName << ">\n";
	// }
	// llvm::outs() << decl->getFileName() << "\n";
      }
    }
  }
}

std::vector<std::string> GenerateIncludeHandler::getAllI() {
  return _AllInc;
}
