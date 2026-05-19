#include "ConsumerMatcher.hpp"
#include "Handler.hpp"

void ConsumerMatcher::HandleTranslationUnit(
  clang::ASTContext &Context) {
  clang::ast_matchers::MatchFinder MatchFinder;
  Handler                          Handler;
  MatchFinder.addMatcher(clang::ast_matchers::functionDecl(), &Handler);
  MatchFinder.matchAST(Context);
}
