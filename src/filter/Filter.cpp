#include "include/Filter.h"

#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/string_view.hpp>
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
#include <cstdint>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <string_view>
#include <vector>

CountNodesVisitor::CountNodesVisitor(clang::ASTContext *C) :
  _C(C),
  _mgr(&(C->getSourceManager()))
    /*_allFunctions(boost::json::object({{"noFunction", "numFunctions"}})) { //,*/
{
    /*_allFunctions() { //,*/
  /*_allFunctions["noFunction"] = {};*/
  _allFunctions = boost::json::object();
  _allFunctions.insert("Program");
  _allFunctions.at("Program") = boost::json::object();
  _allFunctions.at("Program").as_object().insert("numFunctions");
  _allFunctions.at("Program").at("numFunctions") = (0);
}

bool CountNodesVisitor::incrementCount(std::vector<std::string_view> fields) {
  /*std::cout << boost::json::serialize(_allFunctions) << std::endl;*/
  boost::json::object *currentObj = &_allFunctions;
  size_t size = fields.size();
  for (size_t i=0; i<size; i++) {
    std::string_view field = fields[i];
    if (boost::json::value *next = currentObj->if_contains(field)) {
      if (next->is_object()) {
	currentObj = &next->as_object();
      } else if (next->is_uint64() && i == size - 1) {
	next->as_uint64()++;
	return true;
      } else {
	return false;
      }
    } else {
      currentObj->insert(field);
      if (i == size - 1) {
	currentObj->at(field) = uint64_t(1);
      } else {
	currentObj->at(field) = boost::json::object();
	currentObj = &currentObj->at(field).as_object();
      }
    }
  }
  return false;
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
  return "Program";
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
      incrementCount({currentFunc, "Variables", "numInteger"});
    } else if (VD->getType()->isFloatingType()) {
      incrementCount({currentFunc, "Variables", "numFloats"});
    } else if (VD->getType()->isPointerType()) {
      incrementCount({currentFunc, "Variables", "numPointers"});
      /*VD->dumpColor();*/
      std::cout << currentFunc << std::endl;
    } else if (VD->getType()->isStructureType()) {
      incrementCount({currentFunc, "Variables", "numStructs"});
    }
  }
  /*if (VD->getType()->isFunctionType()) incrementCount("numFunctions")++;*/
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitVarDecl(VD);
}

bool CountNodesVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (!FD) return false;
  if (_mgr->isInMainFile(FD->getLocation())) {
    _allFunctions.insert(FD->getNameAsString());
    _allFunctions.at(FD->getNameAsString()) = boost::json::object();
    _allFunctions.at("Program").at("numFunctions").get_int64()++;
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
      incrementCount({currentFunc, "VariableReference", "numIntDeclRef"});
      /*D->dumpColor();*/
      if (partOfBinCompOp(*D))
        incrementCount({currentFunc, "VariableReference", "numIntCompare"});
    } else if (d->isArrayType()) {
      incrementCount({currentFunc, "VariableReference", "numArray"});
      /*D->dumpColor();*/
    } else if (d->isStructureType())
      incrementCount({currentFunc, "VariableReference", "numStructRef"});
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
      incrementCount({currentFunc, "numFunctionCalls"});
    } else if (className == clang::Stmt::UnaryOperatorClass) {
      incrementCount({currentFunc, "Operations", "numOperations"});
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitStmt(S);
}

bool CountNodesVisitor::VisitIntegerLiteral(clang::IntegerLiteral *S) {
  if (!S) return false;
  if (_mgr->isInMainFile(S->getLocation())) {
    if (partOfBinCompOp(*S)) {
      incrementCount({getStmtParentFuncName(*S), "BinaryCompare", "numIntCompare"});
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIntegerLiteral(S);
}

bool CountNodesVisitor::VisitIfStmt(clang::IfStmt *If) {
  if (!If) return false;
  if (_mgr->isInMainFile(If->getIfLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*If);
    incrementCount({currentFunc, "numIfStmt"});
    /*If->dumpColor();*/
    if (If->getCond()->getExprStmt()->getType()->isIntegerType()) {
      // TODO this is almost always true due to being the result of the if
      // not the types being compared
      // which in c is a int not a bool
      incrementCount({currentFunc, "BinaryCompare", "numIfStmtInt"});
    }
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitIfStmt(If);
}

bool CountNodesVisitor::VisitForStmt(clang::ForStmt *F) {
  if (!F) return false;
  if (_mgr->isInMainFile(F->getForLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*F);
    incrementCount({currentFunc, "Loops", "numFor"});
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitForStmt(F);
}

bool CountNodesVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (!W) return false;
  if (_mgr->isInMainFile(W->getWhileLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*W);
    incrementCount({currentFunc, "Loops", "numWhile"});
  }
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitWhileStmt(W);
}

bool CountNodesVisitor::VisitUnaryOperator(clang::UnaryOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getOperatorLoc())) {
    std::string currentFunc = CountNodesVisitor::getStmtParentFuncName(*O);
    incrementCount({currentFunc, "UnaryOperations", "numUnaryOpGeneral"});
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
    incrementCount({currentFunc, "BinaryOperations", "numBinaryOpGeneral"});
    if (O->isComparisonOp()) {
      incrementCount({currentFunc, "BinaryOperations", "numCompareOp"});
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
    incrementCount({currentFunc, "BinaryOperations", "numConditionOp"});
  }
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitConditionalOperator(O);
}

bool CountNodesVisitor::VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O) {
  if (!O) return false;
  if (_mgr->isInMainFile(O->getExprLoc())) {
    std::string currentFunc = getStmtParentFuncName(*O);
    incrementCount({currentFunc, "BinaryOperations", "numConditionOp"});
  }
    return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitBinaryConditionalOperator(O);
}

bool CountNodesVisitor::VisitType(clang::Type *T) {
  if (!T) return false;
  return clang::RecursiveASTVisitor<CountNodesVisitor>::VisitType(T);
}

boost::json::object CountNodesVisitor::Report() {
  return _allFunctions;
}

void CountNodesVisitor::PrintReport() {
  std::cout << boost::json::serialize(_allFunctions) << std::endl;
  std::cout << "{" << std::endl;
  std::string indent = "  ";
  for (const boost::json::key_value_pair &val : _allFunctions) {
    std::string open = (val.value().is_uint64()) ? " : " : " {\n";
    std::cout << indent << val.key() << open;
    PrintReport(val.value(), indent);
    std::cout << indent << ((open == " {\n") ? "}," : "") << std::endl; 
  }
  std::cout << "}" << std::endl;
}

void CountNodesVisitor::PrintReport(const boost::json::value &jv, std::string indent) {
  /*std::cout << boost::json::serialize(*_allFunctions) << std::endl;*/
  indent += "  ";
  if (jv.is_uint64()) {
    std::cout << " " << jv.as_uint64() << ",";
  } else if (jv.is_object()) {
    const boost::json::object *obj = &jv.as_object();
    for (const boost::json::key_value_pair &val : *obj) {
      std::string open = (val.value().is_uint64()) ? " : " : " {\n";
      std::cout << indent << val.key() << open;
      PrintReport(val.value(), indent);
      std::cout << indent << ((open == " {\n") ? "}," : "") << std::endl; 
    }
  } else {
    std::cout << indent << "UNKNOWN," << std::endl;
  }
}
