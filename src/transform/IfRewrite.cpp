#include "include/IfRewrite.hpp"
#include <clang/Basic/SourceManager.h>

IfRewriteVisitor::IfRewriteVisitor(clang::Rewriter &R, clang::ASTContext *C)
    : _Rewrite(R), _Context(C) {}

// May not have needed to create this
bool IfRewriteVisitor::VisitDecl(clang::Decl *D) {
  /*if (_Rewrite.getSourceMgr().isInMainFile(D->getBeginLoc())) {*/
    /*D->dumpColor();*/
  /*}*/
  clang::RecursiveASTVisitor<IfRewriteVisitor>::VisitDecl(D);
  return true;
}

// May not have needed to create this
bool IfRewriteVisitor::VisitType(clang::Type *T) {
  /*if (_Rewrite.getSourceMgr().isInMainFile(T->getAsTagDecl()->getLocation())) {*/
    /*T->dump();*/
  /*}*/
  clang::RecursiveASTVisitor<IfRewriteVisitor>::VisitType(T);
  return true;
}

bool IfRewriteVisitor::VisitStmt(clang::Stmt *S) {
  /*if (_Rewrite.getSourceMgr().isInMainFile(S->getBeginLoc())) {*/
    /*S->dumpColor();*/
  /*}*/
  clang::RecursiveASTVisitor<IfRewriteVisitor>::VisitStmt(S);
  return true;
} 

/*bool IfRewriteVisitor::VisitStmt(clang::Stmt *S) {*/
/*  if (S->getStmtClass() == clang::Stmt::IfStmtClass) {*/
/*    S->dumpPretty(*_Context);*/
/*    S->dumpColor();*/
/*    llvm::outs() << S->getStmtClass() << "\n";*/
/*    clang::IfStmt *I = dyn_cast<clang::IfStmt>(S);*/
/*    if (_Rewrite.getSourceMgr().isInMainFile(I->getIfLoc())) {*/
/*      llvm::outs() << I->getID(*_Context) << " : " << I->getIfLoc().printToString(_Rewrite.getSourceMgr()) << "\n";*/
/*      llvm::outs() << "Look ma YES Match\n";*/
/*      int length0 = _Rewrite.getRangeSize(I->getSourceRange());*/
/*      _Rewrite.ReplaceText(I->getBeginLoc(), length0, "newString");*/
/*      _Rewrite.ReplaceText(I->getIfLoc(), length0, "newString");*/
/*      _Rewrite.InsertText(I->getBeginLoc(), "Wrong");*/
/*      int length1 = _Rewrite.getRangeSize(I->getCond()->getSourceRange());*/
/*      _Rewrite.ReplaceText(I->getCond()->getBeginLoc(), length1, "newString");*/
/*      _Rewrite.InsertText(I->getCond()->getEndLoc(), "Wrong");*/
/*      _Rewrite.InsertText(I->getThen()->getBeginLoc(), "Wrong");*/
/*      _Rewrite.InsertText(I->getThen()->getEndLoc(), "Wrong");*/
/*      auto lp = I->getLParenLoc();*/
/*      llvm::outs() << "Makes it through the Rewrite Stmts\n";*/
/*      clang::RecursiveASTVisitor<IfRewriteVisitor>::VisitStmt(I);*/
/*    } else {*/
/*      llvm::outs() << "Look ma NO Match\n";*/
/*    }*/
/*    llvm::outs() << "Running Return\n";*/
/*    return clang::RecursiveASTVisitor<IfRewriteVisitor>::VisitStmt(I);*/
/*  } else {*/
/*    clang::RecursiveASTVisitor<IfRewriteVisitor>::VisitStmt(S);*/
/*  }*/
/*  clang::RecursiveASTVisitor<IfRewriteVisitor>::VisitStmt(S);*/
/*  return true;*/
/*}*/


bool IfRewriteVisitor::VisitIfStmt(clang::IfStmt *I) {
  /*llvm::outs() << "At least it triggered\n";*/
  clang::SourceManager &mgr = _Rewrite.getSourceMgr();
  if (mgr.isInMainFile(I->getIfLoc())) {
    I->dumpColor();
    llvm::outs() << "Look ma YES Match\n";
    /*_Rewrite.ReplaceText(I->getIfLoc(), "ifLoc:");*/
    /*_Rewrite.RemoveText(I->getIfLoc(), "ifLoc:");*/
    /*_Rewrite.InsertText(I->getCond()->getExprStmt(), "rParen:");*/
    _Rewrite.InsertText(I->getIfLoc(), "ifLoc:");
    _Rewrite.InsertText(I->getBeginLoc(), "ifBegin:");
    _Rewrite.InsertText(I->getEndLoc(), "ifEnd:");
    _Rewrite.InsertText(I->getLParenLoc(), "lParen:");
    _Rewrite.InsertText(I->getRParenLoc(), "rParen:");
    if (clang::Stmt *E = I->getElse()) {
      _Rewrite.InsertText(I->getElseLoc(), "elseLoc:");
      _Rewrite.InsertText(E->getBeginLoc(), "elseBegin:");
      _Rewrite.InsertText(E->getEndLoc(), "elseEnd:");
    }
    if (clang::Stmt *C = I->getCond()) {
      _Rewrite.InsertText(C->getBeginLoc(), "condBegin:");
      _Rewrite.InsertText(C->getEndLoc(), "condEnd:");
      _Rewrite.InsertText(I->getCond()->getExprLoc(), "rParen:");
    }
    if (clang::Stmt *T = I->getThen()) {
      _Rewrite.InsertText(T->getBeginLoc(), "thenBegin:");
      _Rewrite.InsertText(T->getEndLoc(), "thenEnd:");
    } else {
      llvm::outs() << "Look ma NO Match\n";
    }
  }
  /*llvm::outs() << "Running Return\n";*/
  /*return clang::RecursiveASTVisitor<IfRewriteVisitor>::VisitStmt(I);*/
  return clang::RecursiveASTVisitor<IfRewriteVisitor>::VisitIfStmt(I);
  // this seg faults with an overload of mem I think
  /*return IfRewriteVisitor::VisitIfStmt(I);*/
}
