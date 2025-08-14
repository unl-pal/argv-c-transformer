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
            // std::string verifierString = "__VERIFIER_nondet_";
            std::string verifierString = "";
            if (refDecl->declarationReplaces(func)) {
              clang::QualType returnType = E->getCallReturnType(*_C);
              clang::QualType newReturnType = returnType;
              std::string myType = returnType.getAsString();
              std::string newName = "";
              newName += verifierString;
              // std::string returnTypeName = E->getCallReturnType(*_C).getAsString();
              std::string returnTypeName = returnType.getAsString();
              std::string newReturnTypeName = "";
              bool isPointer = returnType->isPointerType();
              if (returnType->isBooleanType()) {
                newReturnType = _C->BoolTy;
                newReturnTypeName = "bool";
              } else if (isPointer) {
                newReturnType = _C->VoidPtrTy;
                myType = "void*";
                newReturnTypeName = "pointer";
              }else {
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
              isPointer ? newName += "(" + returnTypeName + ")(" : newName;
              newName += "__VERIFIER_nondet_" + newReturnTypeName + "()";
              isPointer ? newName += ")" : newName;
              clang::SourceRange range;
              range.setBegin(E->getBeginLoc());
              range.setEnd(E->getEndLoc());
              clang::IdentifierInfo *funcName = &_C->Idents.get("__VERIFIER__non_det_" + newReturnTypeName);
              clang::DeclarationName declName(funcName);
              clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
                *_C,
                _C->getTranslationUnitDecl(),
                E->getExprLoc(),
                E->getExprLoc().getLocWithOffset(1),
                funcName,
                refDecl->getReturnType(),
                // returnType,
                _C->CreateTypeSourceInfo(_C->VoidTy),
                clang::SC_Extern
              );
              if (range.isValid()) {
                std::string verifierString = "";
                llvm::raw_string_ostream tempStream(verifierString);
                isPointer ? verifierString += "(" + newReturnTypeName + ")(" : verifierString;
                newFunction->printName(tempStream);
                isPointer ? verifierString += "())" : verifierString += "()";
                llvm::outs() << verifierString << " - The String for Replace Dead Calls Visitor\n";
                _Rewriter.ReplaceText(E->getSourceRange(), verifierString);
                llvm::outs() << "Inserted the text\n";
              }
              _NeededTypes->emplace(returnType);
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
  // return false;
}
