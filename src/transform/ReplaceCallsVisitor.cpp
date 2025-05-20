#include "include/ReplaceCallsVisitor.hpp"

#include <algorithm>
#include <clang/Basic/SourceManager.h>

ReplaceDeadCallsVisitor::ReplaceDeadCallsVisitor(clang::ASTContext *C,
                                                 clang::Rewriter   &R)
    : _C(C), _R(R) {};

bool ReplaceDeadCallsVisitor::VisitDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitDecl(D);
}

// Call Expr is the parent of the function decl ref and the args used
bool ReplaceDeadCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (_C->getSourceManager().isInMainFile(E->getExprLoc())) {
    if (clang::FunctionDecl *func = E->getCalleeDecl()->getAsFunction()) {
        if (((func->isImplicit() || !func->isDefined())) && !func->isInlineDefinitionExternallyVisible()) {
          std::string myType = func->getReturnType().getAsString();
          std::replace(myType.begin(), myType.end(), ' ', '_');
          clang::IdentifierInfo *newInfo = &_C->Idents.get("__VERIFIER_nondet_" + myType);
          clang::DeclarationName newName(newInfo);
          func->setDeclName(newName);
          E->shrinkNumArgs(0);
          _R.ReplaceText(E->getSourceRange(), newName.getAsString() + "()");
        }
    }
  }
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitCallExpr(E);
}
