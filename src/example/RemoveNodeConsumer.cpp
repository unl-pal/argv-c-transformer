#include "RemoveNodeConsumer.hpp"
#include "RemoveNodeHandler.hpp"

// TODO change to removal node
// look for the nodes that filter leaves behind, the calls with no definition
//
void RemoveNodeConsumer::HandleTranslationUnit(
  clang::ASTContext &Context) {
  clang::ast_matchers::MatchFinder MatchFinder;
  Handler                          Handler;
  MatchFinder.addMatcher(clang::ast_matchers::functionDecl(), &Handler);
  MatchFinder.matchAST(Context);
}
