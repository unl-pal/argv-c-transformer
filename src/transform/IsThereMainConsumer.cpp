#include "AddMainVisitor.hpp"
#include "AllFunctionsToCallHandler.hpp"
#include "IsThereMainConsumer.hpp"
#include "IsThereMainHandler.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>

IsThereMainConsumer::IsThereMainConsumer(clang::Rewriter &rewriter) : _Rewriter(rewriter) 
{
}

using namespace clang::ast_matchers;
void IsThereMainConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  llvm::outs() << "Looking for Main Function\n";
  clang::ast_matchers::MatchFinder MatchFinder;
  IsThereMainHandler Handler;
  context.getTranslationUnitDecl()->dumpColor();
  MatchFinder.addMatcher(decl().bind("main"), &Handler);
  llvm::outs() << "Add Ze Mache\n";
  MatchFinder.matchAST(context);
  llvm::outs() << "Run Ze Mache\n";
  if (!Handler.HasMain()) {
    llvm::outs() << "Not Haz Ze Maene\n";
    clang::ast_matchers::MatchFinder FunctionsMatchFinder;
    AllFunctionsToCallHandler FunctionsHandler;
    FunctionsMatchFinder.addMatcher(functionDecl().bind("functions"), &FunctionsHandler);
    FunctionsMatchFinder.matchAST(context);
    llvm::outs() << "Gotz Nameze\n";
    if (FunctionsHandler.GetNames().size()) {
      AddMainVisitor Visitor(&context, FunctionsHandler.GetNames(), _Rewriter);
      Visitor.VisitTranslationUnitDecl(context.getTranslationUnitDecl());
    }
  } 
}
