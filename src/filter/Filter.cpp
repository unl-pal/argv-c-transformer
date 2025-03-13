#include "include/Filter.h"

#include <clang/AST/ASTContext.h>
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
#include <clang/Lex/PreprocessingRecord.h>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <unordered_map>
#include <utility>

CountNodesVisitor::CountNodesVisitor(clang::ASTContext *C) :
  _C(C),
  _mgr(&(C->getSourceManager())),
  _allFunctions(),
  _isInBinCompOp(false)
{
  _allFunctions = std::unordered_map<std::string, CountNodesVisitor::attributes*>();
  _allFunctions.try_emplace("Program", new attributes);
}

/// check if a stmt is a part of a binary operation
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

/// Take Advantage of built in Decl get Parent function
std::string CountNodesVisitor::getDeclParentFuncName(const clang::Decl &D) {
  std::string currentFunc = "Program";
  if (const clang::DeclContext *parentFuncContext = D.getParentFunctionOrMethod()) {
    if (parentFuncContext->isFunctionOrMethod()) {
      const clang::FunctionDecl *FD = clang::dyn_cast<clang::FunctionDecl>(parentFuncContext);
      currentFunc = FD->getNameAsString();
    }
  } else {
    currentFunc = "Program";
  }
  return currentFunc;
}

/// Stmt does not have get parent function so recurse to decl and use built in
/// from there
std::string CountNodesVisitor::getStmtParentFuncName(const clang::Stmt &S) {
  clang::DynTypedNodeList parents = _C->getParents(S);
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
  return "Program";
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
      _allFunctions[currentFunc]->numVarInt++;
    } else if (VD->getType()->isFloatingType()) {
      _allFunctions[currentFunc]->numVarFloat++;
    } else if (VD->getType()->isPointerType()) {
      _allFunctions[currentFunc]->numVarPoint++;
      /*std::cout << currentFunc << std::endl;*/
    } else if (VD->getType()->isStructureType()) {
      _allFunctions[currentFunc]->numVarStruct++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitVarDecl(VD);
}

bool CountNodesVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (!FD) return false;
  if (_mgr->isInMainFile(FD->getLocation())) {
    _allFunctions.try_emplace(FD->getNameAsString(), new attributes);
    _allFunctions["Program"]->numFunctions++;
    /*FD->dumpColor();*/
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitFunctionDecl(FD);
}

bool CountNodesVisitor::VisitDeclRefExpr(clang::DeclRefExpr *D) {
  if (_mgr->isInMainFile(D->getLocation())) {
    const clang::QualType &d = D->getType();
    std::string currentFunc = getStmtParentFuncName(*D);
    if (d->isIntegerType()) {
      _allFunctions[currentFunc]->numVarRefInt++;
      /*D->dumpColor();*/
      /*std::cout << _isInBinCompOp << std::endl;*/
      if (_isInBinCompOp)
        _allFunctions[currentFunc]->numCompInt++;
    } else if (d->isArrayType()) {
      _allFunctions[currentFunc]->numVarRefArray++;
      /*D->dumpColor();*/
    } else if (d->isStructureType())
      _allFunctions[currentFunc]->numVarRefStruct++;
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
      _allFunctions[currentFunc]->numCallFunc++;
    } else if (className == clang::Stmt::UnaryOperatorClass) {
      _allFunctions[currentFunc]->numOpUnary++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitStmt(S);
}

bool CountNodesVisitor::VisitIntegerLiteral(clang::IntegerLiteral *S) {
  if (!S) return false;
  if (_mgr->isInMainFile(S->getLocation())) {
    if (_isInBinCompOp) {
      _allFunctions[getStmtParentFuncName(*S)]->numCompInt++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIntegerLiteral(S);
}

bool CountNodesVisitor::VisitIfStmt(clang::IfStmt *If) {
  if (!If) return false;
  if (_mgr->isInMainFile(If->getIfLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*If);
    _allFunctions[currentFunc]->numIfStmt++;
    /*std::cout << "If Bool Count: " << _isInBinCompOp << std::endl;*/
    /*If->dumpColor();*/
    if (If->getCond()->getExprStmt()->getType()->isIntegerType()) {
      // TODO this is almost always true due to being the result of the if
      // not the types being compared
      // which in c is a int not a bool
      _allFunctions[currentFunc]->numIfStmtInt++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIfStmt(If);
}

bool CountNodesVisitor::VisitForStmt(clang::ForStmt *F) {
  if (!F) return false;
  if (_mgr->isInMainFile(F->getForLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*F);
    _allFunctions[currentFunc]->numLoopFor++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitForStmt(F);
}

bool CountNodesVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (!W) return false;
  if (_mgr->isInMainFile(W->getWhileLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*W);
    _allFunctions[currentFunc]->numLoopWhile++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitWhileStmt(W);
}

bool CountNodesVisitor::VisitUnaryOperator(clang::UnaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    _allFunctions[currentFunc]->numOpUnary++;
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
    _allFunctions[currentFunc]->numOpBinary++;
    if (O->isComparisonOp()) {
      _allFunctions[currentFunc]->numOpCompare++;
      bool result;
      _isInBinCompOp++;
      /*std::cout << "BinCompOp Count Before recurse: " << _isInBinCompOp << std::endl;*/
      result = clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryOperator(O);
      /*std::cout << "Result: " << result << std::endl;*/
      _isInBinCompOp--;
      /*std::cout << "BinCompOp Count After recurse: " << _isInBinCompOp << std::endl;*/
      return result;
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
    _allFunctions[currentFunc]->numOpCondition++;
  }
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitConditionalOperator(O);
}

bool CountNodesVisitor::VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getExprLoc())) {
    std::string currentFunc = getStmtParentFuncName(*O);
    _allFunctions[currentFunc]->numOpCondition++;
  }
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryConditionalOperator(O);
}

bool CountNodesVisitor::VisitType(clang::Type *T) {
  if (!T) return false;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitType(T);
}

std::unordered_map<std::string, CountNodesVisitor::attributes *>
CountNodesVisitor::ReportAttributes() {
  return _allFunctions;
}

void CountNodesVisitor::PrintReport(std::string fileName) {
  std::cout << fileName << std::endl;
  for (const std::pair<std::string, attributes*> func : _allFunctions) {
    std::cout << " " << func.first << std::endl;
    std::cout << "  numCallFunc: " << func.second->numCallFunc << std::endl;
    std::cout << "  numCompChar: " << func.second->numCompChar << std::endl;
    std::cout << "  numCompFloat: " << func.second->numCompFloat << std::endl;
    std::cout << "  numCompInt: " << func.second->numCompInt << std::endl;
    std::cout << "  numFunctions: " << func.second->numFunctions << std::endl;
    std::cout << "  numIfStmt: " << func.second->numIfStmt << std::endl;
    std::cout << "  numIfStmtInt: " << func.second->numIfStmtInt << std::endl;
    std::cout << "  numLoopFor: " << func.second->numLoopFor << std::endl;
    std::cout << "  numLoopWhile: " << func.second->numLoopWhile << std::endl;
    std::cout << "  numOpBinary: " << func.second->numOpBinary << std::endl;
    std::cout << "  numOpCompare: " << func.second->numOpCompare << std::endl;
    std::cout << "  numOpCondition: " << func.second->numOpCondition << std::endl;
    std::cout << "  numOpUnary: " << func.second->numOpUnary << std::endl;
    std::cout << "  numVarFloat: " << func.second->numVarFloat << std::endl;
    std::cout << "  numVarInt: " << func.second->numVarInt << std::endl;
    std::cout << "  numVarPoint: " << func.second->numVarPoint << std::endl;
    std::cout << "  numVarRefArray: " << func.second->numVarRefArray << std::endl;
    std::cout << "  numVarRefCompare: " << func.second->numVarRefCompare << std::endl;
    std::cout << "  numVarRefInt: " << func.second->numVarRefInt << std::endl;
    std::cout << "  numVarRefStruct: " << func.second->numVarRefStruct << std::endl;
    std::cout << "  numVarStruct: " << func.second->numVarStruct << std::endl;
  }
}
