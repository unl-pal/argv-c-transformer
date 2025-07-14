#include "IsThereMainHandler.hpp"
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

IsThereMainHandler::IsThereMainHandler() {}

void IsThereMainHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &results) {
  if (const clang::FunctionDecl *decl = results.Nodes.getNodeAs<clang::FunctionDecl>("main")) {
    if (decl->isMain()) {
      _hasMain = true;
    }
  }
}

bool IsThereMainHandler::HasMain() {
  return _hasMain;
}
