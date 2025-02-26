#include "include/Filter.h"
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Basic/TypeTraits.h>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <utility>

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
    for (auto& entry : _values) {
      _allFunctions[_currentFunc][entry.first] = entry.second;
    }
}

std::string CountNodesVisitor::getDeclParentFuncName(const clang::Decl &D) {
  clang::DynTypedNodeList parents = _C->getParents(D);
  /*std::cout << parents.size() << std::endl;*/
  if (parents.size()) {
    for (const clang::DynTypedNode& parent : parents) {
      if (const clang::FunctionDecl *fd = parent.get<clang::FunctionDecl>()) {
	parent.dump(llvm::outs(), *_C);
	return fd->getNameAsString();
      }
      else if (const clang::Decl *d = parent.get<clang::Decl>()) {
	return getDeclParentFuncName(*d);
      }
    }
  }
  return "noFunction";
}

std::string CountNodesVisitor::getStmtParentFuncName(const clang::Stmt &S) {
  clang::DynTypedNodeList parents = _C->getParents(S);
  /*std::cout << parents.size() << std::endl;*/
  if (parents.size()) {
    for (const clang::DynTypedNode& parent : parents) {
      if (const clang::FunctionDecl *fd = parent.get<clang::FunctionDecl>()) {
	return fd->getNameAsString();
      }
      else if (const clang::Stmt *s = parent.get<clang::Stmt>()) {
	return getStmtParentFuncName(*s);
      }
    }
  }
  return "noFunction";
}

bool CountNodesVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitDecl(D);
}


bool CountNodesVisitor::VisitVarDecl(clang::VarDecl *VD) {
  if (!VD) return false;
  if (_mgr->isInMainFile(VD->getLocation())) {
    _currentFunc = getDeclParentFuncName(*VD);
    if (VD->getType()->isIntegerType()) _allFunctions[_currentFunc]["numInts"]++;
    if (VD->getType()->isFloatingType()) _allFunctions[_currentFunc]["numFloats"]++;
    /*if (VD->getType()->isFunctionType()) _allFunctions[_currentFunc]["numFunctions"]++;*/
    if (VD->getType()->isPointerType()) _allFunctions[_currentFunc]["numPointers"]++;
    if (VD->getType()->isStructureType()) _allFunctions[_currentFunc]["numStructs"]++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitVarDecl(VD);
}

bool CountNodesVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (!FD) return false;
  if (_mgr->isInMainFile(FD->getLocation())) {
    _allFunctions[getDeclParentFuncName(*FD)]["numFunctions"]++;
    std::string name = FD->getName().str();
    if (_allFunctions.find(name) == _allFunctions.end()) _allFunctions[name] = _values;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitFunctionDecl(FD);
}

bool CountNodesVisitor::VisitStmt(clang::Stmt *S) {
  if (!S) return false;
  if (_mgr->isInMainFile(S->getBeginLoc())) {
    _currentFunc = CountNodesVisitor::getStmtParentFuncName(*S);
    clang::Stmt::StmtClass className = S->getStmtClass();
    if (className == clang::Stmt::CallExprClass) _allFunctions[_currentFunc]["numFunctionCalls"]++;
    else if (className == clang::Stmt::UnaryOperatorClass) _allFunctions[_currentFunc]["numOperations"]++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitStmt(S);
}

bool CountNodesVisitor::VisitIfStmt(clang::IfStmt *If) {
  if (!If) return false;
  if (_mgr->isInMainFile(If->getIfLoc())) {
    _currentFunc = CountNodesVisitor::getStmtParentFuncName(*If);
    _allFunctions[_currentFunc]["numIfStmt"]++;
    If->dumpColor();
    if (If->getCond()->getExprStmt()->getType()->isIntegerType()) {
      // TODO this is almost always true due to being the result of the if
      // not the types being compared
      // which in c is a int not a bool
      _allFunctions[_currentFunc]["numIfStmtInt"]++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIfStmt(If);
}

bool CountNodesVisitor::VisitForStmt(clang::ForStmt *F) {
  if (!F) return false;
  if (_mgr->isInMainFile(F->getForLoc())) {
    _currentFunc = CountNodesVisitor::getStmtParentFuncName(*F);
    _allFunctions[_currentFunc]["numForLoops"]++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitForStmt(F);
}

bool CountNodesVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (!W) return false;
  if (_mgr->isInMainFile(W->getWhileLoc())) {
    _currentFunc = CountNodesVisitor::getStmtParentFuncName(*W);
    _allFunctions[_currentFunc]["numWhileLoops"]++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitWhileStmt(W);
}

bool CountNodesVisitor::VisitUnaryOperator(clang::UnaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    _currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    _allFunctions[_currentFunc]["numUnaryOp"]++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitUnaryOperator(O);
}

bool CountNodesVisitor::VisitBinaryOperator(clang::BinaryOperator *O) {
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    _currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    _allFunctions[_currentFunc]["numBinaryOp"]++;
  }
  if (!O) return false;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryOperator(O);
}

bool CountNodesVisitor::VisitConditionalOperator(clang::ConditionalOperator *O) {
  if (_mgr->isInMainFile(O->getExprLoc())) {
    _currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    _allFunctions[_currentFunc]["numConditionOp"]++;
  }
  if (!O) return false;
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitConditionalOperator(O);
}

bool CountNodesVisitor::VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O) {
  if (_mgr->isInMainFile(O->getExprLoc())) {
    _currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    _allFunctions[_currentFunc]["numBinConditionOp"]++;
  }
  if (!O) return false;
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryConditionalOperator(O);
}

bool CountNodesVisitor::VisitType(clang::Type *T) {
  if (!T) return false;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitType(T);
}

std::map<std::string, std::map<std::string, int>> CountNodesVisitor::report() {
  return _allFunctions;
}

void CountNodesVisitor::PrintReport() {
  for (const std::pair<std::string, std::map<std::string, int>> func : _allFunctions) {
    std::cout << func.first << std::endl;
    for (auto& entry : func.second) {
      std::cout << "  " << entry.first << ": " << entry.second << std::endl;
    }
  }
}
