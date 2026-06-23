#include "include/CountingVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Basic/TypeTraits.h>

CountingVisitor::CountingVisitor(
    clang::ASTContext *C, const std::vector<unsigned int> &T,
    std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> allFunctions)
    : _C(C), _mgr(&(C->getSourceManager())), _allFunctions(allFunctions), _T(T),
      _allTypes(T.empty()) {
  _allFunctions->try_emplace("Program");
}

CountingVisitor::attributes &CountingVisitor::entryFor(const std::string &name) {
  auto it = _allFunctions->find(name);
  if (it == _allFunctions->end())
    it = _allFunctions->find("Program"); // always present (emplaced in ctor)
  return it->second;
}

bool CountingVisitor::matchesType(clang::QualType QT) const {
  if (_allTypes)
    return true;
  for (unsigned int t : _T) {
    if (QT->isSpecificBuiltinType(t))
      return true;
  }
  return false;
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

bool CountingVisitor::VisitDecl(clang::Decl *D) {
  if (!D)
    return false;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitDecl(D);
}

bool CountingVisitor::VisitVarDecl(clang::VarDecl *VD) {
  if (!VD)
    return false;
  if (_mgr->isInMainFile(VD->getLocation())) {
    if (matchesType(VD->getType()))
      entryFor(getDeclParentFuncName(*VD)).TypeVariables++;
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitVarDecl(VD);
}

bool CountingVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (!FD)
    return false;
  if (_mgr->isInMainFile(FD->getLocation())) {
    if (!_allFunctions->count(FD->getNameAsString())) {
      _allFunctions->try_emplace(FD->getNameAsString());
      _allFunctions->at("Program").Functions++;
    }
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitFunctionDecl(FD);
}

bool CountingVisitor::VisitDeclRefExpr(clang::DeclRefExpr *S) {
  if (_mgr->isInMainFile(S->getLocation())) {
    if (matchesType(S->getType()))
      entryFor(getStmtParentFuncName(*S)).TypeVariableReference++;
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitDeclRefExpr(S);
}

bool CountingVisitor::VisitStmt(clang::Stmt *S) {
  if (!S)
    return false;
  if (_mgr->isInMainFile(S->getBeginLoc())) {
    if (S->getStmtClass() == clang::Stmt::CallExprClass)
      entryFor(getStmtParentFuncName(*S)).CallFunc++;
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitStmt(S);
}

bool CountingVisitor::VisitIfStmt(clang::IfStmt *If) {
  if (!If)
    return false;
  if (_mgr->isInMainFile(If->getIfLoc())) {
    std::string currentFunc = getStmtParentFuncName(*If);
    entryFor(currentFunc).IfStmt++;
    if (matchesType(If->getCond()->getType()))
      entryFor(currentFunc).TypeIfStmt++;
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitIfStmt(If);
}

bool CountingVisitor::VisitForStmt(clang::ForStmt *F) {
  if (!F)
    return false;
  if (_mgr->isInMainFile(F->getForLoc()))
    entryFor(getStmtParentFuncName(*F)).ForLoops++;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitForStmt(F);
}

bool CountingVisitor::VisitWhileStmt(clang::WhileStmt *W) {
  if (!W)
    return false;
  if (_mgr->isInMainFile(W->getWhileLoc()))
    entryFor(getStmtParentFuncName(*W)).WhileLoops++;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitWhileStmt(W);
}

bool CountingVisitor::VisitUnaryOperator(clang::UnaryOperator *O) {
  if (!O)
    return false;
  if (_mgr->isInMainFile(O->getOperatorLoc()) && matchesType(O->getType())) {
    std::string currentFunc = getStmtParentFuncName(*O);
    if (O->isArithmeticOp())
      entryFor(currentFunc).TypeArithmeticOperation++;
    entryFor(currentFunc).TypeUnaryOperation++;
    if (O->isPrefix())
      entryFor(currentFunc).TypePrefix++;
    if (O->isPostfix())
      entryFor(currentFunc).TypePostfix++;
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitUnaryOperator(O);
}

bool CountingVisitor::VisitBinaryOperator(clang::BinaryOperator *O) {
  if (!O)
    return false;
  if (_mgr->isInMainFile(O->getOperatorLoc()) && matchesType(O->getType())) {
    std::string currentFunc = getStmtParentFuncName(*O);
    if (O->isAdditiveOp())
      entryFor(currentFunc).TypeArithmeticOperation++;
    if (O->isComparisonOp()) {
      entryFor(currentFunc).TypeCompareOperation++;
      return clang::RecursiveASTVisitor<CountingVisitor>::VisitBinaryOperator(O);
    }
  }
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitBinaryOperator(O);
}

bool CountingVisitor::VisitConditionalOperator(clang::ConditionalOperator *O) {
  if (!O)
    return false;
  if (_mgr->isInMainFile(O->getExprLoc()) && matchesType(O->getType()))
    entryFor(getStmtParentFuncName(*O)).TypeCompareOperation++;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitConditionalOperator(O);
}

bool CountingVisitor::VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O) {
  if (!O)
    return false;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitBinaryConditionalOperator(O);
}

bool CountingVisitor::VisitImplicitParamDecl(clang::ImplicitParamDecl *D) {
  if (matchesType(D->getType()))
    entryFor(getDeclParentFuncName(*D)).TypeParameters++;
  return clang::RecursiveASTVisitor<CountingVisitor>::VisitImplicitParamDecl(D);
}
