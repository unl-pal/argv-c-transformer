#include "include/CountingVisitor.hpp"

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

// Visitor for counting propeties and functions in a file
CountNodesVisitor::CountNodesVisitor(clang::ASTContext *C) :
  _C(C),
  _mgr(&(C->getSourceManager())),
  _allFunctions(),
  _isInBinCompOp(false)
{
  _allFunctions = std::unordered_map<std::string, CountNodesVisitor::attributes*>();
  _allFunctions.try_emplace("Program", new attributes);
}

// check if a stmt is a part of a binary operation
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

// Take Advantage of built in Decl get Parent function
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

// Stmt does not have get parent function so recurse to decl and use built in
// from there
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

// Base Visit Decl, currently used as catch all for unhandled decl types
bool CountNodesVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitDecl(D);
}

// Visits variable declarations and checks if in main file before adding to the
// count of variables for the function or program as a whole if defined outside
// of a function
// can use the defined outside function to check if part of overall
bool CountNodesVisitor::VisitVarDecl(clang::VarDecl *VD) {
  if (!VD) return false;
  if (_mgr->isInMainFile(VD->getLocation())) {
    if (VD->getType()->isIntegerType()) {
      _allFunctions[getDeclParentFuncName(*VD)]->TypeVariables++;
    } else if (VD->getType()->isFloatingType()) {
      _allFunctions[getDeclParentFuncName(*VD)]->VarFloat++;
    } else if (VD->getType()->isPointerType()) {
      _allFunctions[getDeclParentFuncName(*VD)]->VarPoint++;
    } else if (VD->getType()->isStructureType()) {
      _allFunctions[getDeclParentFuncName(*VD)]->StructVariable++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitVarDecl(VD);
}

// Visit function declarations and add to the map of functions and attributes
bool CountNodesVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (!FD) return false;
  if (_mgr->isInMainFile(FD->getLocation())) {
    _allFunctions.try_emplace(FD->getNameAsString(), new attributes);
    _allFunctions["Program"]->Functions++;
    /*FD->dumpColor();*/
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitFunctionDecl(FD);
}

// Visit a declaration reference expression checking for type of variable
// referenced rather than what specfic variable was referenced
// DeclRefExpr is an Expression not Declaration
bool CountNodesVisitor::VisitDeclRefExpr(clang::DeclRefExpr *S) {
  if (_mgr->isInMainFile(S->getLocation())) {
    const clang::QualType &d = S->getType();
    if (d->isIntegerType()) {
      _allFunctions[getStmtParentFuncName(*S)]->TypeVariableReference++;
      if (_isInBinCompOp)
        _allFunctions[getStmtParentFuncName(*S)]->TypeComparisons++;
    } else if (d->isArrayType()) {
      _allFunctions[getStmtParentFuncName(*S)]->VarRefArray++;
    } else if (d->isStructureType())
      _allFunctions[getStmtParentFuncName(*S)]->VarRefStruct++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitDeclRefExpr(S);
}

// Base visit statement call, need to separate out the specific calls if possible
bool CountNodesVisitor::VisitStmt(clang::Stmt *S) {
  if (!S) return false;
  if (_mgr->isInMainFile(S->getBeginLoc())) {
    clang::Stmt::StmtClass className = S->getStmtClass();
    if (className == clang::Stmt::CallExprClass) {
      _allFunctions[getStmtParentFuncName(*S)]->CallFunc++;
    } else if (className == clang::Stmt::UnaryOperatorClass) {
      _allFunctions[getStmtParentFuncName(*S)]->OpUnary++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitStmt(S);
}

// Visit Integer literal, check if part of a binary operation and add to count
// of ints that are a part of a computation
bool CountNodesVisitor::VisitIntegerLiteral(clang::IntegerLiteral *S) {
  if (!S) return false;
  if (_mgr->isInMainFile(S->getLocation())) {
    if (_isInBinCompOp) {
      _allFunctions[getStmtParentFuncName(*S)]->TypeComparisons++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIntegerLiteral(S);
}

// check if 'if' statement is in main file, is a part of a function and add to
// current function count
bool CountNodesVisitor::VisitIfStmt(clang::IfStmt *If) {
  if (!If) return false;
  if (_mgr->isInMainFile(If->getIfLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*If);
    _allFunctions[currentFunc]->IfStmt++;
    if (If->getCond()->getExprStmt()->getType()->isIntegerType()) {
      // TODO this is almost always true due to being the result of the if
      // not the types being compared
      // which in c is a int not a bool
      _allFunctions[currentFunc]->TypeIfStmt++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIfStmt(If);
}

// Visit for loops and add to the function count of for loops
bool CountNodesVisitor::VisitForStmt(clang::ForStmt *F) {
  if (!F) return false;
  if (_mgr->isInMainFile(F->getForLoc())) {
    _allFunctions[getStmtParentFuncName(*F)]->ForLoops++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitForStmt(F);
}

// Visit while loops and add to the function count of while loops
bool CountNodesVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (!W) return false;
  if (_mgr->isInMainFile(W->getWhileLoc())) {
    _allFunctions[getStmtParentFuncName(*W)]->WhileLoops++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitWhileStmt(W);
}

// check for operations that involve only one variable or literal
// TODO match the argv on this
bool CountNodesVisitor::VisitUnaryOperator(clang::UnaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    // if (O->isArithmeticOp()) {
    if (O->isArithmeticOp()) {
      _allFunctions[currentFunc]->TypeArithmeticOperation++;
    }
    _allFunctions[currentFunc]->OpUnary++;
    if (O->isPrefix()) {
      _allFunctions[currentFunc]->Prefix++;
    }
    if (O->isPostfix()) {
      _allFunctions[currentFunc]->Postfix++;
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitUnaryOperator(O);
}

// Visit binary operations, operations with a left and right side, and add to
// the count of total binary operations then check if is a comparison binary
// operation
bool CountNodesVisitor::VisitBinaryOperator(clang::BinaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    std::string currentFunc = getStmtParentFuncName(*O);
    _allFunctions[currentFunc]->OpBinary++;
    if (O->isComparisonOp()) {
      _allFunctions[currentFunc]->OpCompare++;
      // TODO more debug statements
      return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryOperator(O);
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryOperator(O);
}

// Visit conditional operator adding to the parent function count of
// conditional operations
bool CountNodesVisitor::VisitConditionalOperator(clang::ConditionalOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getExprLoc())) {
    _allFunctions[getStmtParentFuncName(*O)]->OpCondition++;
  }
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitConditionalOperator(O);
}

// Visit conditional operators that have a left and right side then add to the count
bool CountNodesVisitor::VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getExprLoc())) {
    _allFunctions[getStmtParentFuncName(*O)]->OpCondition++;
  }
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryConditionalOperator(O);
}

// TODO figure out if types could/should be used on the left and right sides
bool CountNodesVisitor::VisitType(clang::Type *T) {
  if (!T) return false;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitType(T);
}

// count the parameters in the function signiture and check if is an int
bool CountNodesVisitor::VisitImplicitParamDecl(clang::ImplicitParamDecl *D) {
  std::string funcName = getDeclParentFuncName(*D);
  _allFunctions[funcName]->Param++;
  if (D->getType()->isIntegerType()) {
    _allFunctions[funcName]->TypeParameters++;
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitImplicitParamDecl(D);
}

// Getter for all functions and their attributes
std::unordered_map<std::string, CountNodesVisitor::attributes *>
CountNodesVisitor::ReportAttributes() {
  return _allFunctions;
}

// Outdated debugging print statement for the report
void CountNodesVisitor::PrintReport(std::string fileName) {
  std::cout << fileName << std::endl;
  for (const std::pair<std::string, attributes*> func : _allFunctions) {
    std::cout << " " << func.first << std::endl;
    std::cout << "  CallFunc: " << func.second->CallFunc << std::endl;
    std::cout << "  CompChar: " << func.second->CompChar << std::endl;
    std::cout << "  CompFloat: " << func.second->CompFloat << std::endl;
    std::cout << "  CompInt: " << func.second->TypeComparisons << std::endl;
    std::cout << "  Functions: " << func.second->Functions << std::endl;
    std::cout << "  IfStmt: " << func.second->IfStmt << std::endl;
    std::cout << "  IfStmtInt: " << func.second->TypeIfStmt << std::endl;
    std::cout << "  LoopFor: " << func.second->ForLoops << std::endl;
    std::cout << "  LoopWhile: " << func.second->WhileLoops << std::endl;
    std::cout << "  OpBinary: " << func.second->OpBinary << std::endl;
    std::cout << "  OpCompare: " << func.second->OpCompare << std::endl;
    std::cout << "  OpCondition: " << func.second->OpCondition << std::endl;
    std::cout << "  OpUnary: " << func.second->OpUnary << std::endl;
    std::cout << "  VarFloat: " << func.second->VarFloat << std::endl;
    std::cout << "  VarInt: " << func.second->TypeVariables << std::endl;
    std::cout << "  VarPoint: " << func.second->VarPoint << std::endl;
    std::cout << "  VarRefArray: " << func.second->VarRefArray << std::endl;
    std::cout << "  VarRefCompare: " << func.second->VarRefCompare << std::endl;
    std::cout << "  VarRefInt: " << func.second->TypeVariableReference << std::endl;
    std::cout << "  VarRefStruct: " << func.second->VarRefStruct << std::endl;
    std::cout << "  VarStruct: " << func.second->StructVariable << std::endl;
  }
}
