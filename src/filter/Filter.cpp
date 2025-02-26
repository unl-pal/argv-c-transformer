#include "include/Filter.h"
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <iostream>

CountNodesVisitor::CountNodesVisitor(clang::ASTContext *C) :
  _C(C),
  _mgr(&(C->getSourceManager())),
	_values(),
  _allFunctions(),
  _currentFunc("noFunction") {
  _values["numBinaryOp"] = 0;
  _values["numBinConditionOp"] = 0;
  _values["numCompareBool"] = 0;
  _values["numCompareFloat"] = 0;
  _values["numCompareInt"] = 0;
  _values["numCompareObject"] = 0;
  _values["numCompareString"] = 0;
  _values["numConditionOp"] = 0;
  _values["numFloats"] = 0;
  _values["numForLoops"] = 0;
  _values["numFunctionCalls"] = 0;
  _values["numFunctions"] = 0;
  _values["numIfStmt"] = 0;
  _values["numIfStmtInt"] = 0;
  _values["numInts"] = 0;
  _values["numOperations"] = 0;
  _values["numPointers"] = 0;
  _values["numStrings"] = 0;
  _values["numStructs"] = 0;
  _values["numUnaryOp"] = 0;
  _values["numWhileLoops"] = 0;
}

bool CountNodesVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
  /*std::cout << "Decl" << std::endl;*/
  /*if (_mgr->isInMainFile(D->getLocation()) || D->isCanonicalDecl()) {*/
    /*D->dumpColor();*/
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitDecl(D);
  /*}*/
  /*return true;*/
}


bool CountNodesVisitor::VisitVarDecl(clang::VarDecl *VD) {
  /*std::cout << "Var Decl" << std::endl;*/
  if (!VD) return false;
  if (_mgr->isInMainFile(VD->getLocation())) {
    if (VD->getType()->isIntegerType()) _values["numInts"]++;
    if (VD->getType()->isFloatingType()) _values["numFloats"]++;
    /*if (VD->getType()->isFunctionType()) _values["numFunctions"]++;*/
    if (VD->getType()->isPointerType()) _values["numPointers"]++;
    if (VD->getType()->isStructureType()) _values["numStructs"]++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitVarDecl(VD);
}

bool CountNodesVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (!FD) return false;
  /*std::cout << "Function" << std::endl;*/
  if (_mgr->isInMainFile(FD->getLocation())) {
    _values["numFunctions"]++;
    std::string name = FD->getName().str();
    _currentFunc = name;
    _allFunctions[name] = _values;
  }
  /*FD->getDeclContext()->removeDecl(FD);*/
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitFunctionDecl(FD);
}

bool CountNodesVisitor::VisitStmt(clang::Stmt *S) {
  /*std::cout << "Stmt" << std::endl;*/
  if (!S) return false;
  auto className = S->getStmtClass();
  if (className == clang::Stmt::CallExprClass) _values["numFunctionCalls"]++;
  else if (className == clang::Stmt::UnaryOperatorClass) _values["numOperations"]++;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitStmt(S);
}

bool CountNodesVisitor::VisitIfStmt(clang::IfStmt *If) {
  /*std::cout << "If Stmt" << std::endl;*/
  if (!If) return false;
  if (_mgr->isInMainFile(If->getIfLoc())) {
    _values["numIfStmt"]++;
    /*if (If->getCond()->getExprStmt()->getType()->isCharType()) _values["numIfStmt"]++;*/
    if (If->getCond()->getExprStmt()->getType()->isIntegerType()) _values["numIfStmtInt"]++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIfStmt(If);
}

bool CountNodesVisitor::VisitForStmt(clang::ForStmt *F) {
  /*std::cout << "For Stmt" << std::endl;*/
  if (!F) return false;
  if (_mgr->isInMainFile(F->getForLoc())) _values["numForLoops"]++;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitForStmt(F);
}

bool CountNodesVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  /*std::cout << "While Stmt" << std::endl;*/
  if (!W) return false;
  /*W->dumpColor();*/
  /*W->dumpPretty(*_C);*/
  if (_mgr->isInMainFile(W->getWhileLoc())) _values["numWhileLoops"]++;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitWhileStmt(W);
}

bool CountNodesVisitor::VisitUnaryOperator(clang::UnaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) _values["numUnaryOp"]++;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitUnaryOperator(O);
}

bool CountNodesVisitor::VisitBinaryOperator(clang::BinaryOperator *O) {
  if (_mgr->isInMainFile(O->getOperatorLoc())) _values["numBinaryOp"]++;
  if (!O) return false;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryOperator(O);
}

bool CountNodesVisitor::VisitConditionalOperator(clang::ConditionalOperator *O) {
  if (_mgr->isInMainFile(O->getExprLoc())) _values["numConditionOp"]++;
  if (!O) return false;
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitConditionalOperator(O);
}

bool CountNodesVisitor::VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O) {
  if (_mgr->isInMainFile(O->getExprLoc())) _values["numBinConditionOp"]++;
  if (!O) return false;
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryConditionalOperator(O);
}

bool CountNodesVisitor::VisitType(clang::Type *T) {
  /*std::cout << "Type" << std::endl;*/
  if (!T) return false;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitType(T);
}

std::map<std::string, int> CountNodesVisitor::report() {
  return _values;
}

void CountNodesVisitor::PrintReport() {
  for (auto& entry : _values) {
    std::cout << entry.first << ": " << entry.second << std::endl;
  }
}
