#include "include/GenerateIncludeHandler.hpp"
#include <algorithm>

void
GenerateIncludeHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  const clang::Decl *decl =
    Result.Nodes.getNodeAs<clang::Decl>("");
  if (decl->isReferenced() || decl->isUsed()) {
    std::string fileName = _Mgr.getFilename(decl->getLocation()).str();
    if (std::find(_AllInc.begin(), _AllInc.end(), fileName) == _AllInc.end()) {
      _AllInc.push_back(fileName);
      _Output << "#include <" << fileName << ">\n";
    }
  }
}

std::vector<std::string> GenerateIncludeHandler::getAllI() {
  return _AllInc;
}
