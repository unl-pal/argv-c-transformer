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
  if (const clang::DeclContext *parentFuncContext = D.getParentFunctionOrMethod()) {
    if (parentFuncContext->isFunctionOrMethod()) {
      const clang::FunctionDecl *FD = clang::dyn_cast<clang::FunctionDecl>(parentFuncContext);
      return FD->getNameAsString();
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
      if (const clang::FunctionDecl *fd = parent.get<clang::FunctionDecl>())
        return fd->getNameAsString();
      if (const clang::Stmt *s = parent.get<clang::Stmt>())
        return getStmtParentFuncName(*s);
      if (const clang::Decl *d = parent.get<clang::Decl>())
        return getDeclParentFuncName(*d);
    }
  }
  return "FileScope";
}

bool CountingVisitor::VisitCallExpr(clang::CallExpr *CE) {
  // Only count direct CallExpr, not subclasses, to match the original
  // getStmtClass()-based tally.
  if (!_mgr->isInMainFile(CE->getBeginLoc()))
    return true;
  // Verifier nondet / havoc-helper calls stand in for code the transform
  // removed; they are not complexity of the function's own logic, so they
  // never count toward CallFunc (matters when the verify stage re-counts a
  // havocked file).
  if (const clang::FunctionDecl *callee = CE->getDirectCallee()) {
    if (callee->getIdentifier() && isVerifierGenerated(callee->getNameAsString()))
      return true;
  }
  if (CE->getStmtClass() == clang::Stmt::CallExprClass)
    _allFunctions->at(getStmtParentFuncName(*CE)).Complexity.CallFunc++;
  // assumes all thread/concurrency calls will have types from concurrency related headers
  for (const clang::Expr *arg : CE->arguments()) {
    clang::QualType argType = arg->getType();
    if (argType->isPointerType())
      argType = argType->getPointeeType();
    auto info = stdHeaderForType(argType.getUnqualifiedType().getAsString());
    if (info && info->category == HeaderCategory::Concurrency) {
      _allFunctions->at(getStmtParentFuncName(*CE)).Features.Concurrency = true;
      break;
    }
  }
  return true;
}

bool CountingVisitor::VisitVarDecl(clang::VarDecl *VD) {
  if (_mgr->isInMainFile(VD->getLocation())) {
    clang::QualType varType = VD->getType();
    if (varType->isPointerType())
      varType = varType->getPointeeType();
    if (varType->isFloatingType())
      _allFunctions->at(getDeclParentFuncName(*VD)).Features.FloatingPoint = true;
    // check for concurrency
    auto info = stdHeaderForType(varType.getUnqualifiedType().getAsString());
    if (info && info->category == HeaderCategory::Concurrency)
      _allFunctions->at(getDeclParentFuncName(*VD)).Features.Concurrency = true;
  }
  return true;
}

bool CountingVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (_mgr->isInMainFile(FD->getLocation())) {
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
  if (_mgr->isInMainFile(If->getIfLoc()))
    _allFunctions->at(getStmtParentFuncName(*If)).Complexity.IfStmt++;
  return true;
}

bool CountingVisitor::VisitForStmt(clang::ForStmt *F) {
  if (_mgr->isInMainFile(F->getForLoc()))
    _allFunctions->at(getStmtParentFuncName(*F)).Complexity.ForLoops++;
  return true;
}

bool CountingVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (_mgr->isInMainFile(W->getWhileLoc()))
    _allFunctions->at(getStmtParentFuncName(*W)).Complexity.WhileLoops++;
  return true;
}

bool CountingVisitor::VisitBinaryOperator(clang::BinaryOperator *B) {
  // Only signed overlfow is UB
  if (_mgr->isInMainFile(B->getOperatorLoc()) && B->getType()->isSignedIntegerType()) {
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
  if (_mgr->isInMainFile(U->getOperatorLoc()) && U->getType()->isSignedIntegerType()) {
    if (U->canOverflow())
      _allFunctions->at(getStmtParentFuncName(*U)).Complexity.Operations++;
  }
  return true;
}
