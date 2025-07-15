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
void IsThereMainConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  llvm::outs() << "Looking for Main Function\n";
  clang::ast_matchers::MatchFinder MatchFinder;
  IsThereMainHandler Handler(_Rewriter);
  // TODO REMOVE ME
  if (Context.Idents.find("main") != Context.Idents.end()) {
    auto lookup = Context.Idents.find("main");
    llvm::outs() << "Found Lookup\n";
    // if (lookup) {
      // llvm::outs() << "Found main name\n";
      if (clang::DeclarationName main = lookup->second) {
        llvm::outs() << "Found main info\n";
        if (clang::TranslationUnitDecl *TD = Context.getTranslationUnitDecl()) {
          llvm::outs() << "Found TD\n";
          if (clang::FunctionDecl *func = TD->lookup(main).find_first<clang::FunctionDecl>()) {
            llvm::outs() << "Found main\n";
            func->dumpColor();
          }
        }
      }
    // }
  }
  // context.getTranslationUnitDecl()->dumpColor();
  // MatchFinder.addMatcher(decl().bind("something"), &Handler);
  MatchFinder.addMatcher(functionDecl().bind("functions"), &Handler);
  // llvm::outs() << "Add Ze Mache\n";
  MatchFinder.matchAST(Context);
  // llvm::outs() << "Run Ze Mache\n";
  // if (!Handler.HasMain()) {
    // llvm::outs() << "Not Haz Ze Maene\n";
  //   clang::ast_matchers::MatchFinder FunctionsMatchFinder;
  //   AllFunctionsToCallHandler FunctionsHandler;
  //   FunctionsMatchFinder.addMatcher(functionDecl().bind("functions"), &FunctionsHandler);
  //   FunctionsMatchFinder.matchAST(Context);
  //   llvm::outs() << "Gotz Nameze\n";
  //   if (FunctionsHandler.GetNames().size()) {
  //     AddMainVisitor Visitor(&Context, FunctionsHandler.GetNames(), _Rewriter);
  //     Visitor.VisitTranslationUnitDecl(Context.getTranslationUnitDecl());
  //   }
  // } 
}
