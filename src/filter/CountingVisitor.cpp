// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/CountingVisitor.hpp"
#include "StdHeaders.hpp"
#include "VerifierNames.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Basic/TypeTraits.h>

CountingVisitor::CountingVisitor(
    clang::ASTContext *C,
    std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> allFunctions)
    : _C(C), _mgr(&(C->getSourceManager())), _allFunctions(allFunctions) {
  _allFunctions->try_emplace("FileScope");
}

std::string CountingVisitor::getDeclParentFuncName(const clang::Decl &D) {
  // Walk past anything that isn't a FunctionDecl to find the nearest
  // real enclosing function, rather than assuming the cast succeeds.
  for (const clang::DeclContext *ctx = D.getParentFunctionOrMethod(); ctx;
       ctx = clang::Decl::castFromDeclContext(ctx)->getParentFunctionOrMethod()) {
    if (const clang::FunctionDecl *FD = clang::dyn_cast<clang::FunctionDecl>(ctx)) {
      std::string name = FD->getNameAsString();
      // guarantee function entry exists, may add harmless irrelevant entries
      _allFunctions->try_emplace(name);
      return name;
    }
  }
  return "FileScope";
}

std::string CountingVisitor::getStmtParentFuncName(const clang::Stmt &S) {
  // getParents() returns a list because template instantiations can have
  // multiple parents; in practice we take the first match.
  clang::DynTypedNodeList parents = _C->getParents(S);
  if (parents.size()) {
    for (const clang::DynTypedNode &parent : parents) {
      // DynTypedNode is type-erased - try each possible parent kind
      if (const clang::FunctionDecl *fd = parent.get<clang::FunctionDecl>()) {
        std::string name = fd->getNameAsString();
        _allFunctions->try_emplace(name); // see getDeclParentFuncName
        return name;
      }
      if (const clang::Stmt *s = parent.get<clang::Stmt>())
        return getStmtParentFuncName(*s);
      if (const clang::Decl *d = parent.get<clang::Decl>())
        return getDeclParentFuncName(*d);
    }
  }
  return "FileScope";
}

bool CountingVisitor::VisitCallExpr(clang::CallExpr *CE) {
  // Only count direct CallExpr, not subclasses
  if (!_mgr->isInMainFile(_mgr->getExpansionLoc(CE->getBeginLoc())))
    return true;
  // don't count Verifier nondet / havoc-helper / reach_error / asserts
  if (const clang::FunctionDecl *callee = CE->getDirectCallee()) {
    if (callee->getIdentifier() && isVerifierGenerated(callee->getNameAsString())) {
      if (callee->getName() == "reach_error")
        _allFunctions->try_emplace("reach_error");
      return true;
    }
  }
  if (CE->getStmtClass() == clang::Stmt::CallExprClass)
    _allFunctions->at(getStmtParentFuncName(*CE)).Complexity.CallFunc++;
  // assumes all thread/concurrency calls will have types from concurrency related headers
  for (const clang::Expr *arg : CE->arguments()) {
    clang::QualType argType = arg->getType();
    if (argType->isPointerType())
      argType = argType->getPointeeType();
    auto it = StdHeaders.find(argType.getUnqualifiedType().getAsString());
    if (it != StdHeaders.end() &&
        (it->second == "pthread.h" || it->second == "threads.h" || it->second == "semaphore.h")) {
      _allFunctions->at(getStmtParentFuncName(*CE)).Features.Concurrency = true;
      break;
    }
  }
  if (const clang::FunctionDecl *callee = CE->getDirectCallee()) {
    if (!callee->getIdentifier() || !_mgr->isInSystemHeader(callee->getBeginLoc())) return true;
    if (callee->getNameAsString() == "free" || callee->getNameAsString() == "munmap") {
      _allFunctions->at(getStmtParentFuncName(*CE)).Features.MemFree = true;
    } else if (isMemoryFunction(callee->getNameAsString())) {
      _allFunctions->at(getStmtParentFuncName(*CE)).Features.MemAlloc = true;
    }
  }
  return true;
}

bool CountingVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(FD->getLocation()))) {
    if (!_allFunctions->count(FD->getNameAsString()))
      _allFunctions->try_emplace(FD->getNameAsString());
    attributes &entry = _allFunctions->at(FD->getNameAsString());
    entry.Complexity.Param = FD->getNumParams();
    if (FD->getReturnType()->isFloatingType())
      entry.Features.FloatingPoint = true;
  }
  return true;
}

bool CountingVisitor::VisitIfStmt(clang::IfStmt *If) {
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(If->getIfLoc())))
    _allFunctions->at(getStmtParentFuncName(*If)).Complexity.IfStmt++;
  return true;
}

bool CountingVisitor::VisitForStmt(clang::ForStmt *F) {
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(F->getForLoc())))
    _allFunctions->at(getStmtParentFuncName(*F)).Complexity.ForLoops++;
  return true;
}

bool CountingVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(W->getWhileLoc())))
    _allFunctions->at(getStmtParentFuncName(*W)).Complexity.WhileLoops++;
  return true;
}

bool CountingVisitor::VisitBinaryOperator(clang::BinaryOperator *B) {
  // Only signed overlfow is UB
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(B->getOperatorLoc())) &&
      B->getType()->isSignedIntegerType()) {
    clang::BinaryOperator::Opcode op =
        B->isCompoundAssignmentOp()
            ? clang::BinaryOperator::getOpForCompoundAssignment(B->getOpcode())
            : B->getOpcode();
    if (clang::BinaryOperator::isMultiplicativeOp(op) || clang::BinaryOperator::isAdditiveOp(op) ||
        clang::BinaryOperator::isShiftOp(op))
      _allFunctions->at(getStmtParentFuncName(*B)).Complexity.Operations++;
  }
  return true;
}

bool CountingVisitor::VisitUnaryOperator(clang::UnaryOperator *U) {
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(U->getOperatorLoc()))) {
    if (U->canOverflow() && U->getType()->isSignedIntegerType())
      _allFunctions->at(getStmtParentFuncName(*U)).Complexity.Operations++;
    if (U->getOpcode() == clang::UO_Deref)
      _allFunctions->at(getStmtParentFuncName(*U)).Features.PointerDeref = true;
  }
  return true;
}

bool CountingVisitor::VisitArraySubscriptExpr(clang::ArraySubscriptExpr *ASE) {
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(ASE->getBeginLoc())))
    _allFunctions->at(getStmtParentFuncName(*ASE)).Features.PointerDeref = true;
  return true;
}

bool CountingVisitor::VisitMemberExpr(clang::MemberExpr *ME) {
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(ME->getExprLoc())) && ME->isArrow())
    _allFunctions->at(getStmtParentFuncName(*ME)).Features.PointerDeref = true;
  return true;
}

bool CountingVisitor::VisitStringLiteral(clang::StringLiteral *SL) {
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(SL->getBeginLoc())))
    _allFunctions->at(getStmtParentFuncName(*SL)).Features.PointerOrArray = true;
  return true;
}

bool CountingVisitor::VisitDeclRefExpr(clang::DeclRefExpr *DRE) {
  if (_mgr->isInMainFile(_mgr->getExpansionLoc(DRE->getLocation()))) {
    clang::QualType type = DRE->getType();
    if (type->isPointerType() || type->isArrayType())
      _allFunctions->at(getStmtParentFuncName(*DRE)).Features.PointerOrArray = true;
    if (type->isPointerType())
      type = type->getPointeeType();
    if (type->isFloatingType())
      _allFunctions->at(getStmtParentFuncName(*DRE)).Features.FloatingPoint = true;
    auto it = StdHeaders.find(type.getUnqualifiedType().getAsString());
    if (it != StdHeaders.end() &&
        (it->second == "pthread.h" || it->second == "threads.h" || it->second == "semaphore.h"))
      _allFunctions->at(getStmtParentFuncName(*DRE)).Features.Concurrency = true;
  }
  return true;
}
