#include "GenerateIncludeConsumer.hpp"
#include "GenerateIncludeHandler.hpp"
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Lex/PreprocessingRecord.h>
#include <ostream>

using namespace clang::ast_matchers;

GenerateIncludeConsumer::GenerateIncludeConsumer(llvm::raw_fd_ostream &output) :
  _Output(output) {}

void GenerateIncludeConsumer::HandleTranslationUnit(
  clang::ASTContext &Context) {
  MatchFinder MatchFinder;
  GenerateIncludeHandler        Handler(Context.getSourceManager(), _Output);
  llvm::outs() << "Created MAtch Finder and Handler" << "\n";
  MatchFinder.addMatcher(decl(unless(clang::ast_matchers::isExpansionInMainFile())).bind("includes"), &Handler);
  // MatchFinder.addMatcher(clang::InclusionDirective().bind("includes"), &Handler);
  llvm::outs() << "Added Matcher" << "\n";
  MatchFinder.matchAST(Context);
  llvm::outs() << "Ran the matcher" << "\n";
  Handler.getAllI();
  llvm::outs() << "grabbed all includes" << "\n";
  std::vector<std::string> allTheIncludes = Handler.getAllI();
  if (allTheIncludes.size()) {
    for (std::string i : allTheIncludes) {
      llvm::outs() << i << "\n";
    }
  }
}
