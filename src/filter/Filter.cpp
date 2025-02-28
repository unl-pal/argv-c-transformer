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
#include <clang/Basic/Specifiers.h>
#include <clang/Basic/TypeTraits.h>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <utility>

// TODO remove _values
CountNodesVisitor::CountNodesVisitor(clang::ASTContext *C) :
  _C(C),
  _mgr(&(C->getSourceManager())),
	/*_values(),*/
  _allFunctions() { //,
  /*_currentFunc("noFunction") {*/
  /*_values["numArray"] = 0;*/
  /*_values["numBinaryOp"] = 0;*/
  /*_values["numBinConditionOp"] = 0;*/
  /*_values["numCompareBool"] = 0;*/
  /*_values["numCompareFloat"] = 0;*/
  /*_values["numCompareInt"] = 0;*/
  /*_values["numCompareObject"] = 0;*/
  /*_values["numCompareString"] = 0;*/
  /*_values["numConditionOp"] = 0;*/
  /*_values["numFloats"] = 0;*/
  /*_values["numForLoops"] = 0;*/
  /*_values["numFunctionCalls"] = 0;*/
  /*_values["numFunctions"] = 0;*/
  /*_values["numIfInt"] = 0;*/
  /*_values["numIfStmt"] = 0;*/
  /*_values["numIfStmtInt"] = 0;*/
  /*_values["numInts"] = 0;*/
  /*_values["numOperations"] = 0;*/
  /*_values["numPointers"] = 0;*/
  /*_values["numStrings"] = 0;*/
  /*_values["numStructs"] = 0;*/
  /*_values["numUnaryOp"] = 0;*/
  /*_values["numWhileLoops"] = 0;*/
  _allFunctions["noFunction"] = std::map<std::string, int>();
}

void CountNodesVisitor::incrementCount(std::string currentFunc, std::string count) {
      if (_allFunctions[currentFunc][count]) {
	_allFunctions[currentFunc][count]++;
      } else {
	  _allFunctions[currentFunc][count] = 1;
      }
}

bool CountNodesVisitor::partOfBinCompOp(const clang::Stmt &S) {
  clang::DynTypedNodeList parents = _C->getParents(S);
  if (parents.size()) {
    const clang::DynTypedNode *parent = parents.begin();
    if (const clang::ImplicitCastExpr *imp =
            parent->get<clang::ImplicitCastExpr>()) {
      clang::DynTypedNodeList grandParents = _C->getParents(*imp);
      if (grandParents.size()) {
          if (const clang::BinaryOperator *gp = grandParents.begin()->get<clang::BinaryOperator>()) {
	  return gp->isComparisonOp();
	}
      }
    } else if (const clang::BinaryOperator *gp = parent->get<clang::BinaryOperator>()) {
      return gp->isComparisonOp();
    }
  }
  return false;
}

std::string CountNodesVisitor::getDeclParentFuncName(const clang::Decl &D) {
  clang::DynTypedNodeList parents = _C->getParents(D);
  /*std::cout << parents.size() << std::endl;*/
  if (parents.size()) {
    for (const clang::DynTypedNode& parent : parents) {
      if (const clang::FunctionDecl *fd = parent.get<clang::FunctionDecl>()) {
	parent.dump(llvm::outs(), *_C);
	return fd->getNameAsString();
      } else if (const clang::Stmt *s = parent.get<clang::Stmt>()) {
	return getStmtParentFuncName(*s);
      } else if (const clang::Decl *d = parent.get<clang::Decl>()) {
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
      } else if (const clang::Stmt *s = parent.get<clang::Stmt>()) {
	return getStmtParentFuncName(*s);
      } else if (const clang::Decl *d = parent.get<clang::Decl>()) {
	return getDeclParentFuncName(*d);
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
    std::string currentFunc = getDeclParentFuncName(*VD);
    if (VD->getType()->isIntegerType()) {
      incrementCount(currentFunc, "numInteger");
    } else if (VD->getType()->isFloatingType()) {
      incrementCount(currentFunc, "numFloats");
    } else if (VD->getType()->isPointerType()) {
      incrementCount(currentFunc, "numPointers");
      /*VD->dumpColor();*/
      std::cout << currentFunc << std::endl;
    } else if (VD->getType()->isStructureType()) {
      incrementCount(currentFunc, "numStructs");
    }
  }
  /*if (VD->getType()->isFunctionType()) incrementCount("numFunctions")++;*/
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitVarDecl(VD);
}

bool CountNodesVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (!FD) return false;
  if (_mgr->isInMainFile(FD->getLocation())) {
    _allFunctions[FD->getNameAsString()] = std::map<std::string, int>();
    _allFunctions[getDeclParentFuncName(*FD)]["numFunctions"]++;
    std::cout << FD->getNameAsString() << std::endl;
    FD->dumpColor();
    /*std::string name = FD->getName().str();*/
    /*if (_allFunctions.find(name) == _allFunctions.end()) _allFunctions[name] = _values;*/
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitFunctionDecl(FD);
}

bool CountNodesVisitor::VisitDeclRefExpr(clang::DeclRefExpr *D) {
  if (_mgr->isInMainFile(D->getLocation())) {
    const clang::QualType &d = D->getType();
    std::string currentFunc = getStmtParentFuncName(*D);
    if (d->isIntegerType()) {
      incrementCount(currentFunc, "numIntDeclRef");
      /*D->dumpColor();*/
      if (partOfBinCompOp(*D))
        incrementCount(currentFunc, "numIntCompare");
    } else if (d->isArrayType()) {
      incrementCount(currentFunc, "numArray");
      /*D->dumpColor();*/
    } else if (d->isStructureType())
      incrementCount(currentFunc, "numStructRef");
    /*if (d->isCharType()) return false;*/
    /*if (d->isDoubleType()) return false;*/
    /*if (d->isFloatingType()) return false;*/
    /*if (d->isBooleanType()) return false;*/
    /*if (d->isStructureType()) return false;*/
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitDeclRefExpr(D);
}

bool CountNodesVisitor::VisitStmt(clang::Stmt *S) {
  if (!S) return false;
  if (_mgr->isInMainFile(S->getBeginLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*S);
    clang::Stmt::StmtClass className = S->getStmtClass();
    if (className == clang::Stmt::CallExprClass) {
      incrementCount(currentFunc, "numFunctionCalls");
    } else if (className == clang::Stmt::UnaryOperatorClass) {
      incrementCount(currentFunc, "numOperations");
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitStmt(S);
}

bool CountNodesVisitor::VisitIntegerLiteral(clang::IntegerLiteral *S) {
  if (!S) return false;
  if (_mgr->isInMainFile(S->getLocation())) {
    if (partOfBinCompOp(*S)) {
      incrementCount(getStmtParentFuncName(*S), "numIntCompare");
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIntegerLiteral(S);
}

bool CountNodesVisitor::VisitIfStmt(clang::IfStmt *If) {
  if (!If) return false;
  if (_mgr->isInMainFile(If->getIfLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*If);
    incrementCount(currentFunc, "numIfStmt");
    /*If->dumpColor();*/
    if (If->getCond()->getExprStmt()->getType()->isIntegerType()) {
      // TODO this is almost always true due to being the result of the if
      // not the types being compared
      // which in c is a int not a bool
      incrementCount(currentFunc, "numIfStmtInt");
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIfStmt(If);
}

bool CountNodesVisitor::VisitForStmt(clang::ForStmt *F) {
  if (!F) return false;
  if (_mgr->isInMainFile(F->getForLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*F);
    incrementCount(currentFunc, "numForLoops");
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitForStmt(F);
}

bool CountNodesVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (!W) return false;
  if (_mgr->isInMainFile(W->getWhileLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*W);
    incrementCount(currentFunc, "numWhileLoops");
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitWhileStmt(W);
}

bool CountNodesVisitor::VisitUnaryOperator(clang::UnaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    incrementCount(currentFunc, "numUnaryOp");
    O->isPrefix();
    O->isPostfix();
    O->isDecrementOp();
    O->isIncrementOp();
    O->isArithmeticOp();
    O->isKnownToHaveBooleanValue();
    O->isLValue();
    O->isPRValue();
    /*O->isNullPointerConstant(*_C, NPC);*/
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitUnaryOperator(O);
}

bool CountNodesVisitor::VisitBinaryOperator(clang::BinaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    std::string currentFunc = getStmtParentFuncName(*O);
    incrementCount(currentFunc, "numBinaryOp");
    if (O->isComparisonOp()) {
      incrementCount(currentFunc, "numBinaryCompareOp");
    }
    /*if (O->isEqualityOp()) {*/
    /*  incrementCount(currentFunc, "numEqualityOp");*/
    /*}*/
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryOperator(O);
}

bool CountNodesVisitor::VisitConditionalOperator(clang::ConditionalOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getExprLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    incrementCount(currentFunc, "numConditionOp");
  }
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitConditionalOperator(O);
}

bool CountNodesVisitor::VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getExprLoc())) {
    std::string currentFunc = getStmtParentFuncName(*O);
    incrementCount(currentFunc, "numBinConditionOp");
  }
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
