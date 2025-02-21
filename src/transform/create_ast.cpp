/*#include "create_ast.hpp"*/

/*#include <clang/Rewrite/Core/Rewriter.h>*/
/*#include <clang/Tooling/Tooling.h>*/
/*#include <clang/ASTMatchers/ASTMatchers.h>*/
/*#include <clang/ASTMatchers/ASTMatchFinder.h>*/
/**/
/*using namespace clang;*/
/*using namespace clang::ast_matchers;*/
/*using namespace clang::tooling;*/
/**/
/**/
/*class MyPrinter : public clang::ast_matchers::MatchFinder::MatchCallback {*/
/*public:*/
/*  MyPrinter(Rewriter &R) : _Rewrite(R) {}*/
/**/
/*  void run(const MatchFinder::MatchResult &Result) {*/
/*    if (const IfStmt *If = Result.Nodes.getNodeAs<IfStmt>("ifStmt")) {*/
/*      // Reconstruct the If statement*/
/*      _Rewrite.InsertText(If->getBeginLoc(), "if (");*/
/*      _Rewrite.InsertText(If->getCond()->getEndLoc(), ") ");*/
/*      _Rewrite.InsertText(If->getThen()->getBeginLoc(), "{");*/
/*      _Rewrite.InsertText(If->getThen()->getEndLoc(), "}");*/
/*      // ... (Handle else branch)*/
/*    }*/
/*  }*/
/**/
/*private:*/
/*  clang::Rewriter &_Rewrite;*/
/*};*/

/*MyPrinter::MyPrinter(Rewriter &R) : _Rewrite(R) {}*/
/**/
/*void MyPrinter::run(const MatchFinder::MatchResult &Result) {*/
/*  if (const IfStmt *If = Result.Nodes.getNodeAs<IfStmt>("ifStmt")) {*/
/*    // Reconstruct the If statement*/
/*    _Rewrite.InsertText(If->getBeginLoc(), "if (");*/
/*    _Rewrite.InsertText(If->getCond()->getEndLoc(), ") ");*/
/*    _Rewrite.InsertText(If->getThen()->getBeginLoc(), "{");*/
/*    _Rewrite.InsertText(If->getThen()->getEndLoc(), "}");*/
/*    // ... (Handle else branch)*/
/*  }*/
/*}*/
