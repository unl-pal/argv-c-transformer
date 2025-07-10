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
        if (_C->getSourceManager().isInMainFile(func->getLocation()) && (!func->isDefined() || func->isImplicit()) && func->getStorageClass() != clang::SC_Extern) {
          clang::FunctionDecl *refDecl = E->getDirectCallee();
          if (refDecl) {
            std::string verifierString = "__VERIFIER_nondet_";
            if (refDecl->declarationReplaces(func)) {
              clang::QualType funcType = E->getCallReturnType(*_C);
              std::string myType = funcType.getAsString();
              std::string newName = "";
              newName += verifierString;
              std::string returnTypeName = E->getCallReturnType(*_C).getAsString();
              std::string newReturnTypeName = "";
              bool isPointer = E->getType()->isPointerType();
              if (returnTypeName == "_Bool") {
                newReturnTypeName = "bool";
              } else {
                for (unsigned i=0; i<returnTypeName.size(); i++) {
                  char letter = returnTypeName[i];
                  if (letter == ' ') {
                    newReturnTypeName += "";
                  } else if (letter == '_') {
                    newReturnTypeName += "";
                  } else if (letter == '*') {
                    newReturnTypeName += "";
                  } else {
                    newReturnTypeName += letter;
                  } 
                }
              }
              isPointer ? newName += "(" + newReturnTypeName : newName;
              isPointer ? newName += "*)(" : newName += "";
              newName += "__VERIFIER_nondet_" + newReturnTypeName + "()";
              isPointer ? newName += ")" : newName;
              _NeededTypes->emplace(funcType);
              _Rewriter.ReplaceText(E->getSourceRange(), newName);
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
