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
        // Some of the c features used to ID dead calla
        if (_C->getSourceManager().isInMainFile(func->getLocation()) && (!func->isDefined() || func->isImplicit()) && func->getStorageClass() != clang::SC_Extern) {
          clang::FunctionDecl *refDecl = E->getDirectCallee();
          if (refDecl) {
            // Clean up discrepancies between ClangAST naming and ArgC src code requirements
            std::string verifierString = "";
            if (refDecl->declarationReplaces(func)) {
              clang::QualType returnType = E->getCallReturnType(*_C);
              clang::QualType newReturnType = returnType;
              std::string myType = returnType.getAsString();
              std::string newName = "";
              newName += verifierString;
              std::string returnTypeName = returnType.getAsString();
              std::string newReturnTypeName = "";
              bool isPointer = returnType->isPointerType();
              if (returnType->isBooleanType()) {
                newReturnType = _C->BoolTy;
                newReturnTypeName = "bool";
              } else if (isPointer) {
                // All pointers are replaced with a generic Void Pointer and cast to the original return type
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
              // If the return value is a pointer it needs to be cast to the original return type
              isPointer ? newName += "(" + returnTypeName + ")(" : newName;
              newName += "__VERIFIER_nondet_" + newReturnTypeName + "()";
              isPointer ? newName += ")" : newName;
              clang::SourceRange range;
              range.setBegin(E->getBeginLoc());
              range.setEnd(E->getEndLoc());

              // Find the verifier function if it exists or add to the needed
              // verifier types if it does not yet exist Even if the verifier
              // function does not exist yet we can still use the meta data to
              // make the replacement call

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

              // Replace the call if the range is valid and writable
              if (range.isValid()) {
                // Use a stringstream and string to take advantage of the built in print to string functionality
                std::string verifierString = "";
                llvm::raw_string_ostream tempStream(verifierString);
                // Add any needed components for casting pointers
                isPointer ? verifierString += "(" + newReturnTypeName + ")(" : verifierString;
                newFunction->printName(tempStream);
                isPointer ? verifierString += "())" : verifierString += "()";
                llvm::outs() << verifierString << " - The String for Replace Dead Calls Visitor\n";
                _Rewriter.ReplaceText(E->getSourceRange(), verifierString);
                llvm::outs() << "Inserted the text\n";
              }
              _NeededTypes->emplace(returnType); // track any missing verifier functions
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
