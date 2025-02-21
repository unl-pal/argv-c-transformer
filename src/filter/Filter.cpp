#include "include/Filter.h"
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <iostream>

CountNodesVisitor::CountNodesVisitor(clang::ASTContext *C) :
  _C(C),
  _mgr(&(C->getSourceManager())),
  _numIfStmt(0),
  _numInts(0),
  _numLoops(0),
  _numOperations(0),
  _numPointers(0) {}

bool CountNodesVisitor::VisitDecl(clang::Decl *D) {
  if (D == nullptr) return false;
  std::cout << "Decl" << std::endl;
  /*if (_mgr->isInMainFile(D->getLocation())) {*/
  if (_C->getSourceManager().isInMainFile(D->getLocation())) {
  /*if (mgr.isWrittenInMainFile(D->getLocation())) {*/
    D->dumpColor();
  }
  /*std::cout << "after dump" << std::endl;*/
  /*else {*/
  /*  return false;*/
  /*}*/
  /*D->dumpColor();*/
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitDecl(D);
  /*clang::RecursiveASTVisitor<CountNodesVisitor>::TraverseDecl(D);*/
  /*return true;*/
}


bool CountNodesVisitor::VisitVarDecl(clang::VarDecl *VD) {
  std::cout << "Var Decl" << std::endl;
  if (VD == nullptr) return false;
  /*if (!_mgr->isInMainFile(VD->getLocation())) return false;*/
  if (_mgr->isInMainFile(VD->getLocation())) {
    if (VD->getType()->isIntegerType()) _numInts++;
    if (VD->getType()->isPointerType()) _numPointers++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitVarDecl(VD);
  /*clang::RecursiveASTVisitor<CountNodesVisitor>::TraverseVarDecl(VD);*/
  /*return true;*/
}

bool CountNodesVisitor::VisitStmt(clang::Stmt *S) {
  std::cout << "Stmt" << std::endl;
  if (S == nullptr) return false;
  /*clang::operator*/
  /*if (!_mgr->isInMainFile(S->get())) return false;*/
  /*if (clang::dyn_cast<clang::IfStmt>(S)) _numIfStmt++;*/
  /*if (clang::dyn_cast<clang::ForStmt>(S)) _numLoops++;*/
  /*if (clang::dyn_cast<clang::WhileStmt>(S)) _numLoops++;*/
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitStmt(S);
  /*clang::RecursiveASTVisitor<CountNodesVisitor>::VisitStmt(S);*/
  /*return true;*/
}

bool CountNodesVisitor::VisitIfStmt(clang::IfStmt *If) {
  std::cout << "If Stmt" << std::endl;
  if (If == nullptr) return false;
  /*if (!_mgr->isInMainFile(If->getIfLoc())) return false;*/
  if (_mgr->isInMainFile(If->getIfLoc())) {
    _numIfStmt++;
    if (If->getElse()) _numIfStmt++;
  }
  /*else if (_mgr->isInMainFile(If->getElseLoc())) _numIfStmt++;*/
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIfStmt(If);
  /*clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIfStmt(If);*/
  /*return true;*/
}

bool CountNodesVisitor::VisitForStmt(clang::ForStmt *F) {
  std::cout << "For Stmt" << std::endl;
  if (F == nullptr) return false;
  /*if (!_mgr->isInMainFile(F->getForLoc())) return false;*/
  if (_mgr->isInMainFile(F->getForLoc())) _numLoops++;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitForStmt(F);
  /*clang::RecursiveASTVisitor<CountNodesVisitor>::VisitForStmt(F);*/
  /*return true;*/
}

bool CountNodesVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  std::cout << "While Stmt" << std::endl;
  if (W == nullptr) return false;
  /*if (!_mgr->isInMainFile(W->getWhileLoc())) return false;*/
  W->dumpColor();
  W->dumpPretty(*_C);
  if (_mgr->isInMainFile(W->getWhileLoc())) _numLoops++;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitWhileStmt(W);
  /*clang::RecursiveASTVisitor<CountNodesVisitor>::VisitWhileStmt(W);*/
  /*return true;*/
}

bool CountNodesVisitor::VisitType(clang::Type *T) {
  std::cout << "Type" << std::endl;
  if (T == nullptr) return false;
  /*if (!_mgr->isInMainFile(T->getAsRecordDecl()->getLocation())) return false;*/
  /*if (_mgr->isInMainFile(T->getAsRecordDecl()->getLocation())) {*/
    /*if (T->isPointerType()) _numPointers++;*/
    /*if (T->isIntegerType()) _numInts++;*/
  /*}*/
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitType(T);
  /*clang::RecursiveASTVisitor<CountNodesVisitor>::VisitType(T);*/
  /*return true;*/
}

std::unordered_map<std::string, int> CountNodesVisitor::report() {
  std::unordered_map<std::string, int> results{{"ifs", _numIfStmt},
                                               {"ints", _numInts},
                                               {"loops", _numLoops},
                                               {"operations", _numOperations},
                                               {"pointers", _numPointers}};
  return results;
}

void CountNodesVisitor::PrintReport() {
  std::cout << "ifs:        " << _numIfStmt << std::endl;
  std::cout << "ints:       " << _numInts << std::endl;
  std::cout << "loops:      " << _numLoops << std::endl;
  std::cout << "operations: " << _numOperations << std::endl;
  std::cout << "pointers:   " << _numPointers << std::endl;
}
