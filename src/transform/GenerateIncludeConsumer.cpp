#include "GenerateIncludeConsumer.hpp"
#include "GenerateIncludeHandler.hpp"
#include <clang/ASTMatchers/ASTMatchers.h>
#include <ostream>

using namespace clang::ast_matchers;

GenerateIncludeConsumer::GenerateIncludeConsumer(llvm::raw_fd_ostream &output) :
  _Output(output) {}

void GenerateIncludeConsumer::HandleTranslationUnit(
  clang::ASTContext &Context) {
  MatchFinder MatchFinder;
  GenerateIncludeHandler        Handler(Context.getSourceManager(), _Output);
  MatchFinder.addMatcher(decl(unless(clang::ast_matchers::isExpansionInMainFile())), &Handler);
  MatchFinder.matchAST(Context);
  Handler.getAllI();
}
