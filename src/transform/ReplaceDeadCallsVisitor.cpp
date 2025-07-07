#include "include/ReplaceDeadCallsVisitor.hpp"

#include <algorithm>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>

ReplaceDeadCallsVisitor::ReplaceDeadCallsVisitor(clang::ASTContext *C, std::set<clang::QualType> *neededTypes, clang::Rewriter &rewriter)
    : _C(C), _NeededTypes(neededTypes), _Rewriter(rewriter) {
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
          clang::FunctionDecl *refDecl = E->getDirectCallee();
          if (refDecl) {
            std::string verifierString = "__VERIFIER_nondet_";
            std::string name = refDecl->getNameAsString();
            if (name.size() > verifierString.size() && name.substr(0, 17) == verifierString) {

            }
            if (refDecl->declarationReplaces(func)) {
              // clang::QualType funcType = refDecl->getReturnType();
              clang::QualType funcType = E->getCallReturnType(*_C);
              std::string myType = funcType.getAsString();
              std::replace(myType.begin(), myType.end(), ' ', '_');
              clang::IdentifierInfo *newInfo = &_C->Idents.get(verifierString + myType);
              clang::DeclarationName newName(newInfo);
              func->setDeclName(newName);
              _NeededTypes->insert(funcType);
              E->shrinkNumArgs(0);
              _Rewriter.ReplaceText(E->getSourceRange(), newName.getAsString() + "()");
            }
          }
        }
      }
    }
  }
  // return true;
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitCallExpr(E);
}

bool ReplaceDeadCallsVisitor::shouldTraversePostOrder() {
  return true;
}
