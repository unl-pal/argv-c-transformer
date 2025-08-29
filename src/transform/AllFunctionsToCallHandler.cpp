#include "AllFunctionsToCallHandler.hpp"
#include <clang/AST/Decl.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <string>
#include <vector>

AllFunctionsToCallHandler::AllFunctionsToCallHandler() 
  : _Names(std::vector<std::string>())
{
}

void AllFunctionsToCallHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &results) {
  if (const clang::FunctionDecl *decl = results.Nodes.getNodeAs<clang::FunctionDecl>("functions")) {
    if (decl->getASTContext().getSourceManager().isInMainFile(decl->getLocation())) {
      if (!decl->isMain()) {
	_Names.push_back(decl->getNameAsString());
      }
    }
  }
}

std::vector<std::string> AllFunctionsToCallHandler::GetNames() {
  return _Names;
}
