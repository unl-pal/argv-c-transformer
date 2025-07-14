#include "AddMainVisitor.hpp"
#include "AllFunctionsToCallHandler.hpp"
#include "IsThereMainConsumer.hpp"
#include "IsThereMainHandler.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <llvm/Support/raw_ostream.h>

IsThereMainConsumer::IsThereMainConsumer() 
{
}

using namespace clang::ast_matchers;
void IsThereMainConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  llvm::outs() << "Looking for Main Function\n";
  clang::ast_matchers::MatchFinder MatchFinder;
  IsThereMainHandler Handler;
  MatchFinder.addMatcher(functionDecl(isMain()).bind("main"), &Handler);
  MatchFinder.matchAST(context);
  if (Handler.HasMain()) {
    clang::ast_matchers::MatchFinder FunctionsMatchFinder;
    AllFunctionsToCallHandler FunctionsHandler;
    FunctionsMatchFinder.addMatcher(functionDecl().bind("functions"), &FunctionsHandler);
    FunctionsMatchFinder.matchAST(context);
    AddMainVisitor Visitor(FunctionsHandler.GetNames());
    Visitor.HandleTranslationUnit(clang::ASTContext context);
  } 
}
