#include "include/MyRewrite.hpp"

MyRewriteMain::MyRewriteMain(clang::Rewriter &R) : _Rewriter(R) {}

void MyRewriteMain::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  if (const clang::IfStmt *If = Result.Nodes.getNodeAs<clang::IfStmt>("ifStmt")) {
    if (Result.SourceManager->isInMainFile(If->getIfLoc()))
    {
      llvm::outs() << "What do it do\n";
      // Reconstruct the If statement
      /*_Rewrite.InsertText(If->getBeginLoc(), "if (");*/
      /*_Rewrite.InsertText(If->getCond()->getEndLoc(), ") ");*/
      /*_Rewrite.InsertText(If->getThen()->getBeginLoc(), "{");*/
      /*_Rewrite.InsertText(If->getThen()->getEndLoc(), "}");*/
      _Rewriter.InsertText(If->getBeginLoc(), "Wrong");
      _Rewriter.InsertText(If->getCond()->getEndLoc(), "Wrong");
      _Rewriter.InsertText(If->getThen()->getBeginLoc(), "Wrong");
      _Rewriter.InsertText(If->getThen()->getEndLoc(), "Wrong");
    // } else {
    //   llvm::outs() << "Not my if statement\n";
    }
  } else {
    llvm::outs() << "It don't do a thing\n";
  }
}

