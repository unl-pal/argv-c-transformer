#include "GenerateIncludeConsumer.hpp"
#include "GenerateIncludeHandler.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Lex/PreprocessingRecord.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang::ast_matchers;

/*
 * THIS CODE MAY NO LONGER BE NEEDED BUT IS BEING KEPT AS REFERENCE MATERIAL
 * UNTIL OTHER EXAMPLES AND CODE EXIST
 */

// clang::ast_matchers::MatchFinder::MatchCallback::onStartOfTranslationUnit

GenerateIncludeConsumer::GenerateIncludeConsumer(llvm::raw_fd_ostream &output) :
  _Output(output) {}

void GenerateIncludeConsumer::HandleTranslationUnit(
  clang::ASTContext &Context) {
  llvm::outs() << "Running the Handle TU of Generate Includes\n";
  if (!Context.getTranslationUnitDecl()) {
    llvm::outs() << "Context is blank\n";
  }
  // Context.getTranslationUnitDecl()->dumpDeclContext();
  // ## SAVING FOR EXAMPLE SAKE BUT WILL NOT BE USED FOR NOW ##
  // MatchFinder MatchFinder;
  // GenerateIncludeHandler        Handler(Context.getSourceManager(), _Output);
  // llvm::outs() << "Created MAtch Finder and Handler" << "\n";
  // MatchFinder.addMatcher(decl(unless(clang::ast_matchers::isExpansionInMainFile())).bind("includes"), &Handler);
  // // MatchFinder.addMatcher(clang::InclusionDirective().bind("includes"), &Handler);
  // llvm::outs() << "Added Matcher" << "\n";
  // MatchFinder.matchAST(Context);
  // llvm::outs() << "Ran the matcher" << "\n";
  // Handler.getAllI();
  // llvm::outs() << "grabbed all includes" << "\n";
  // std::vector<std::string> allTheIncludes = Handler.getAllI();
  // if (allTheIncludes.size()) {
  //   for (std::string i : allTheIncludes) {
  //     llvm::outs() << i << "\n";
  //   }
  // }
}
