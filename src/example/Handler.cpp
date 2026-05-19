#include "include/Handler.hpp"

void
Handler::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  const clang::FunctionDecl *Function =
    Result.Nodes.getNodeAs<clang::FunctionDecl>("root");
  Function->dump();
}
