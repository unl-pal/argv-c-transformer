#include "IsThereMainHandler.hpp"
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

IsThereMainHandler::IsThereMainHandler() : _hasMain(false) {}

void IsThereMainHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &results) {
  if (const clang::FunctionDecl *decl = results.Nodes.getNodeAs<clang::FunctionDecl>("main")) {
    // decl->dumpColor();
    if (decl->isMain()) {
      this->_hasMain = true;
    }
  }
}

bool IsThereMainHandler::HasMain() {
  return this->_hasMain;
}
