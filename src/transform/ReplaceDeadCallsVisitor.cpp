#include "include/ReplaceDeadCallsVisitor.hpp"

#include <algorithm>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/raw_ostream.h>

ReplaceDeadCallsVisitor::ReplaceDeadCallsVisitor(clang::ASTContext *C, std::set<clang::QualType> *neededTypes)
    : _C(C), _NeededTypes(neededTypes) {
};

bool ReplaceDeadCallsVisitor::VisitTranslationUnit(clang::TranslationUnitDecl *D) {
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::TraverseDecl(D);
}

bool ReplaceDeadCallsVisitor::VisitDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitDecl(D);
}

// Find the Call Expressions for the removed functions and update them
bool ReplaceDeadCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (_C->getSourceManager().isInMainFile(E->getExprLoc())) {
    if (clang::Decl *calleeDecl = E->getCalleeDecl()) {
      if (clang::FunctionDecl *func = calleeDecl->getAsFunction()) {
        if (_C->getSourceManager().isInMainFile(func->getLocation()) && (!func->isDefined() || func->isImplicit())) {
          auto refDecl = E->getDirectCallee();
          if (refDecl) {
            if (refDecl->declarationReplaces(func)) {
              clang::QualType funcType = func->getReturnType();
              std::string myType = funcType.getAsString();
              std::replace(myType.begin(), myType.end(), ' ', '_');
              clang::IdentifierInfo *newInfo = &_C->Idents.get("__VERIFIER_nondet_" + myType);
              clang::DeclarationName newName(newInfo);
              func->setDeclName(newName);
              _NeededTypes->insert(funcType);
              E->shrinkNumArgs(0);
            }
          }
        }
      }
    }
  }
  return true;
}

bool ReplaceDeadCallsVisitor::shouldTraversePostOrder() {
  return true;
}
