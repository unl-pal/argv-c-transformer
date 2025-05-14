#include "include/ReplaceCallsVisitor.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>

ReplaceDeadCallsVisitor::ReplaceDeadCallsVisitor(clang::ASTContext *C,
                                                 clang::Rewriter   &R)
    : _C(C), _R(R) {};

bool ReplaceDeadCallsVisitor::VisitDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitDecl(D);
}

// Call Expr is the parent of the function decl ref and the args used
bool ReplaceDeadCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (!_C->getSourceManager().isInMainFile(E->getExprLoc())) return true;
  if (clang::FunctionDecl *func = E->getCalleeDecl()->getAsFunction()) {
    if ((func->isImplicit()) || (!func->isDefined() && !func->isExternC())) {
      std::string myType = func->getReturnType().getAsString();
      clang::IdentifierInfo *newInfo = &_C->Idents.get("__VERIFIER_nondet_"+myType);
      clang::DeclarationName newName(newInfo);
      func->setDeclName(newName);
      E->shrinkNumArgs(0);
      _R.ReplaceText(E->getSourceRange(), newName.getAsString() + "()");
    }
  }
  // return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitCallExpr(E);
  return true;
}
