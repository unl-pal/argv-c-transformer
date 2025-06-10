#include "ConsumerFindInclude.hpp"
#include "HandlerFindInclude.hpp"
#include <clang/ASTMatchers/ASTMatchers.h>
#include <ostream>

using namespace clang::ast_matchers;

ConsumerFindInclude::ConsumerFindInclude(std::ostream &output) :
  _Output(output) {}

void ConsumerFindInclude::HandleTranslationUnit(
  clang::ASTContext &Context) {
  MatchFinder MatchFinder;
  HandlerFindInclude        Handler(Context.getSourceManager(), _Output);
  MatchFinder.addMatcher(decl(unless(clang::ast_matchers::isExpansionInMainFile())), &Handler);
  MatchFinder.matchAST(Context);
  Handler.getAllI();
}
