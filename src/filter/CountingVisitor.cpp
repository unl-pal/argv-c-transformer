#include "include/CountingVisitor.hpp"
#include "StdHeaders.hpp"

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
  _allFunctions->try_emplace("Program");
}

CountingVisitor::attributes &CountingVisitor::entryFor(const std::string &name) {
  auto it = _allFunctions->find(name);
  if (it == _allFunctions->end())
    it = _allFunctions->find("Program"); // always present (emplaced in ctor)
  return it->second;
}

std::string CountingVisitor::getDeclParentFuncName(const clang::Decl &D) {
  if (const clang::DeclContext *parentFuncContext = D.getParentFunctionOrMethod()) {
    if (parentFuncContext->isFunctionOrMethod()) {
      const clang::FunctionDecl *FD = clang::dyn_cast<clang::FunctionDecl>(parentFuncContext);
      return FD->getNameAsString();
    }
  }
  return "Program";
}

std::string CountingVisitor::getStmtParentFuncName(const clang::Stmt &S) {
  // getParents() returns a list because template instantiations can have
  // multiple parents; in practice we take the first match.
  clang::DynTypedNodeList parents = _C->getParents(S);
  if (parents.size()) {
    for (const clang::DynTypedNode &parent : parents) {
      // DynTypedNode is type-erased — try each possible parent kind
      if (const clang::FunctionDecl *fd = parent.get<clang::FunctionDecl>())
        return fd->getNameAsString();
      if (const clang::Stmt *s = parent.get<clang::Stmt>())
        return getStmtParentFuncName(*s);
      if (const clang::Decl *d = parent.get<clang::Decl>())
        return getDeclParentFuncName(*d);
    }
  }
  return "Program";
}

bool CountingVisitor::VisitCallExpr(clang::CallExpr *CE) {
  if (!CE || !_mgr->isInMainFile(CE->getBeginLoc()))
    return clang::RecursiveASTVisitor<CountingVisitor>::VisitCallExpr(CE);
  // assumes all thread/concurrency calls will have types from concurrency related headers
  for (const clang::Expr *arg : CE->arguments()) {
    clang::QualType argType = arg->getType();
    if (argType->isPointerType())
      argType = argType->getPointeeType();
    auto info = stdHeaderForType(argType.getUnqualifiedType().getAsString());
    if (info && info->category == HeaderCategory::Concurrency) {
      entryFor(getStmtParentFuncName(*CE)).Features.Concurrency = true;
      break;
    }
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitCallExpr(CE);
}

bool CountingVisitor::VisitDecl(clang::Decl *D) {
  if (!D)
    return false;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitDecl(D);
}

bool CountingVisitor::VisitVarDecl(clang::VarDecl *VD) {
  if (!VD)
    return false;
  if (_mgr->isInMainFile(VD->getLocation())) {
    clang::QualType varType = VD->getType();
    if (varType->isPointerType())
      varType = varType->getPointeeType();
    if (varType->isFloatingType())
      entryFor(getDeclParentFuncName(*VD)).Features.FloatingPoint = true;
    // check for concurrency
    auto info = stdHeaderForType(varType.getUnqualifiedType().getAsString());
    if (info && info->category == HeaderCategory::Concurrency)
      entryFor(getDeclParentFuncName(*VD)).Features.Concurrency = true;
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitVarDecl(VD);
}

bool CountingVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (!FD)
    return false;
  if (_mgr->isInMainFile(FD->getLocation())) {
    if (!_allFunctions->count(FD->getNameAsString())) {
      _allFunctions->try_emplace(FD->getNameAsString());
      _allFunctions->at("Program").Complexity.Functions++;
    }
    attributes &entry = entryFor(FD->getNameAsString());
    entry.Complexity.Param = FD->getNumParams();
    if (FD->getReturnType()->isFloatingType())
      entry.Features.FloatingPoint = true;
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitFunctionDecl(FD);
}

bool CountingVisitor::VisitStmt(clang::Stmt *S) {
  if (!S)
    return false;
  if (_mgr->isInMainFile(S->getBeginLoc())) {
    if (S->getStmtClass() == clang::Stmt::CallExprClass)
      entryFor(getStmtParentFuncName(*S)).Complexity.CallFunc++;
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitStmt(S);
}

bool CountingVisitor::VisitIfStmt(clang::IfStmt *If) {
  if (!If)
    return false;
  if (_mgr->isInMainFile(If->getIfLoc()))
    entryFor(getStmtParentFuncName(*If)).Complexity.IfStmt++;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitIfStmt(If);
}

bool CountingVisitor::VisitForStmt(clang::ForStmt *F) {
  if (!F)
    return false;
  if (_mgr->isInMainFile(F->getForLoc()))
    entryFor(getStmtParentFuncName(*F)).Complexity.ForLoops++;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitForStmt(F);
}

bool CountingVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (!W)
    return false;
  if (_mgr->isInMainFile(W->getWhileLoc()))
    entryFor(getStmtParentFuncName(*W)).Complexity.WhileLoops++;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitWhileStmt(W);
}
