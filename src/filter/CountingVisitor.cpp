#include "include/CountingVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Basic/TypeTraits.h>
#include <iostream>
#include <llvm/Support/raw_ostream.h>

CountNodesVisitor::CountNodesVisitor(clang::ASTContext *C,
  const std::vector<unsigned int> &T,
  std::unordered_map<std::string, CountNodesVisitor::attributes*> *allFunctions)
  :
  _C(C),
  _mgr(&(C->getSourceManager())),
  _allFunctions(allFunctions),
  _T(T),
  _allTypes(!T.size())
{
  _allFunctions->try_emplace("Program", new attributes);
}

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
    for (unsigned int specificType : _T) {
      if (_allTypes || VD->getType()->isSpecificBuiltinType(specificType)) {
	_allFunctions->at(getDeclParentFuncName(*VD))->TypeVariables++;
	break;
      }
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitVarDecl(VD);
}

bool CountNodesVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (!FD) return false;
  if (_mgr->isInMainFile(FD->getLocation())) {
    _allFunctions->try_emplace(FD->getNameAsString(), new attributes);
    _allFunctions->at("Program")->Functions++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitFunctionDecl(FD);
}

bool CountNodesVisitor::VisitDeclRefExpr(clang::DeclRefExpr *S) {
  if (_mgr->isInMainFile(S->getLocation())) {
    for (unsigned int specificType : _T) {
      if (_allTypes || S->getType()->isSpecificBuiltinType(specificType)) {
	_allFunctions->at(getStmtParentFuncName(*S))->TypeVariableReference++;
      }
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitDeclRefExpr(S);
}

bool CountNodesVisitor::VisitStmt(clang::Stmt *S) {
  if (!S) return false;
  if (_mgr->isInMainFile(S->getBeginLoc())) {
    clang::Stmt::StmtClass className = S->getStmtClass();
    if (className == clang::Stmt::CallExprClass) {
      _allFunctions->at(getStmtParentFuncName(*S))->CallFunc++;
    } else if (className == clang::Stmt::UnaryOperatorClass) {
      _allFunctions->at(getStmtParentFuncName(*S))->TypeUnaryOperation++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitStmt(S);
}

bool CountNodesVisitor::VisitIfStmt(clang::IfStmt *If) {
  if (!If) return false;
  if (_mgr->isInMainFile(If->getIfLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*If);
    _allFunctions->at(currentFunc)->IfStmt++;
    for (unsigned int specificType : _T) {
      if (_allTypes || If->getCond()->getType()->isSpecificBuiltinType(specificType)) {
	_allFunctions->at(currentFunc)->TypeIfStmt++;
      }
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIfStmt(If);
}

bool CountNodesVisitor::VisitForStmt(clang::ForStmt *F) {
  if (!F) return false;
  if (_mgr->isInMainFile(F->getForLoc())) {
    _allFunctions->at(getStmtParentFuncName(*F))->ForLoops++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitForStmt(F);
}

bool CountNodesVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (!W) return false;
  if (_mgr->isInMainFile(W->getWhileLoc())) {
    _allFunctions->at(getStmtParentFuncName(*W))->WhileLoops++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitWhileStmt(W);
}

bool CountNodesVisitor::VisitUnaryOperator(clang::UnaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    for (unsigned int specificType : _T) {
      if (_allTypes || O->getType()->isSpecificBuiltinType(specificType)) {
	if (O->isArithmeticOp()) {
	  _allFunctions->at(currentFunc)->TypeArithmeticOperation++;
	}
	_allFunctions->at(currentFunc)->TypeUnaryOperation++;
	if (O->isPrefix()) {
	  _allFunctions->at(currentFunc)->TypePrefix++;
	}
	if (O->isPostfix()) {
	  _allFunctions->at(currentFunc)->TypePostfix++;
	}
      }
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitUnaryOperator(O);
}

bool CountNodesVisitor::VisitBinaryOperator(clang::BinaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    for (unsigned int specificType : _T) {
      if (_allTypes || O->getType()->isSpecificBuiltinType(specificType)) {
	std::string currentFunc = getStmtParentFuncName(*O);
	if (O->isAdditiveOp()) {
	  _allFunctions->at(currentFunc)->TypeArithmeticOperation++;
	}
	if (O->isComparisonOp()) {
	  _allFunctions->at(currentFunc)->TypeCompareOperation++;
	  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryOperator(O);
	}
      }
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryOperator(O);
}

bool CountNodesVisitor::VisitConditionalOperator(clang::ConditionalOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getExprLoc())) {
    for (unsigned int specificType : _T) {
      if (_allTypes || O->getType()->isSpecificBuiltinType(specificType)) {
	_allFunctions->at(getStmtParentFuncName(*O))->TypeCompareOperation++;
      }
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitConditionalOperator(O);
}

bool CountNodesVisitor::VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getExprLoc())) {
    // _allFunctions->at(getStmtParentFuncName(*O))->OpCondition++;
  }
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryConditionalOperator(O);
}

bool CountNodesVisitor::VisitImplicitParamDecl(clang::ImplicitParamDecl *D) {
  std::string funcName = getDeclParentFuncName(*D);
  for (unsigned int specificType : _T) {
    if (_allTypes || D->getType()->isSpecificBuiltinType(specificType)) {
      _allFunctions->at(funcName)->TypeParameters++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitImplicitParamDecl(D);
}

std::unordered_map<std::string, CountNodesVisitor::attributes *>
CountNodesVisitor::ReportAttributes() {
  return *_allFunctions;
}

void CountNodesVisitor::PrintReport(std::string fileName) {
  std::cout << fileName << std::endl;
  for (const std::pair<std::string, attributes*> func : *_allFunctions) {
    std::cout << " " << func.first << std::endl;
    std::cout << "CallFunctions: " << func.second->CallFunc << std::endl;
    std::cout << "ForLoops: " << func.second->ForLoops << std::endl;
    std::cout << "Functions: " << func.second->Functions << std::endl;
    std::cout << "IfStmt: " << func.second->IfStmt << std::endl;
    std::cout << "Param: " << func.second->Param << std::endl;
    std::cout << "TypeArithmeticOperation: "
              << func.second->TypeArithmeticOperation << std::endl;
    std::cout << "TypeCompareOperation: " << func.second->TypeCompareOperation
              << std::endl;
    std::cout << "TypeComparisons: " << func.second->TypeComparisons << std::endl;
    std::cout << "TypeIfStmt: " << func.second->TypeIfStmt << std::endl;
    std::cout << "TypeParameters: " << func.second->TypeParameters << std::endl;
    std::cout << "TypePostfix: " << func.second->TypePostfix << std::endl;
    std::cout << "TypePrefix: " << func.second->TypePrefix << std::endl;
    std::cout << "TypeUnaryOperation: " << func.second->TypeUnaryOperation << std::endl;
    std::cout << "TypeVariableReference: " << func.second->TypeVariableReference
              << std::endl;
    std::cout << "TypeVariables: " << func.second->TypeVariables << std::endl;
    std::cout << "WhileLoops: " << func.second->WhileLoops << std::endl;
  }
}
