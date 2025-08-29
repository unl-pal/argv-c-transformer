#include "include/RemoveVisitor.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>

RemoveFuncVisitor::RemoveFuncVisitor(clang::ASTContext       *C,
                                     clang::Rewriter         &rewriter,
                                     std::vector<std::string> *toRemove,
                                     std::set<clang::QualType> *neededTypes)
    : _C(C),
      _mgr(rewriter.getSourceMgr()),
      _Rewriter(rewriter),
      _toRemove(toRemove),
      _NeededTypes(neededTypes) {
  llvm::outs() << "To Remove Size: " << _toRemove->size() << "\n";
}

bool RemoveFuncVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if(!D) return false;
  if (_mgr.isInMainFile(D->getLocation())) {
    llvm::outs() << D->getNameAsString() << " is being checked" << "\n";
    for (std::string& name : *_toRemove) {
      if (name == D->getNameAsString() && D->getLocation().isValid() && D->getLocation().isMacroID()) {
        // Do not remove or attempt to edit functions within macros as the loc
        // is an un-writable location
        return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
      }
      // If the function is the main function move on else wise
      if (name == D->getNameAsString() && !D->isMain()) {
        clang::SourceLocation zero = _mgr.translateLineCol(_mgr.getMainFileID(), _mgr.getSpellingLineNumber(D->getLocation()), 1);
        clang::SourceRange range = clang::SourceRange(zero, D->getEndLoc());
        // Delete whole body if present
        if (!D->hasBody()) {
          range.setEnd(D->getEndLoc().getLocWithOffset(1));
        }
        // Check for valid range of function being removed
        if (range.isValid()) {
          llvm::outs() << name << " Is Valid Range" << "\n";
          _Rewriter.RemoveText(range);
          llvm::outs() << name << " Has Been Removed" << "\n";
        }
        // Clear out related comments if present
        clang::RawComment *rawComment = _C->getRawCommentForDeclNoCache(D);
        if (rawComment != nullptr) {
          if (rawComment->getSourceRange().isValid()) {
            _Rewriter.RemoveText(rawComment->getSourceRange());
          }
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
}

bool RemoveFuncVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (_mgr.isInMainFile(E->getExprLoc())) {
    // After all filtered functions are removed we can remove all calls and
    // references to the functions We do it in this step as well as later clean
    // up in transform action due to a loss in context for the function calls
    // that if the AST later evaluates always returns an int as the return value
    // due to how the c language is structured. Everything is an int in the end
    if (clang::FunctionDecl *func = E->getDirectCallee()) {
      std::string name = func->getNameAsString();
      llvm::outs() << name << " call is being checked" << "\n";
      clang::QualType returnType = E->getCallReturnType(*_C);
      for (std::string removedFuncName : *_toRemove) {
        if (name == removedFuncName) {
          // Clean up ClangAST and ArgC name and src code discrepencies
          clang::QualType callReturnType = E->getCallReturnType(*_C);
          std::string newName = "";
          bool isPointer = callReturnType->isPointerType();
          bool isBool = returnType->isBooleanType();
          std::string returnTypeName = callReturnType.getAsString();
          std::string newReturnTypeName = "";
          if (isBool) {
            newReturnTypeName = "bool";
            callReturnType = _C->BoolTy;
          } else if (isPointer) {
            callReturnType = _C->VoidPtrTy;
            newReturnTypeName = "pointer";
          } else {
            newReturnTypeName = "";
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
          if (newReturnTypeName.size()) {
            // Use the pointer logic to set up the casts for verifier pointer to
            // the correct return type for the function called
            isPointer ? newName += "(" + returnTypeName + ")(" : newName;
            newName += "__VERIFIER_nondet_" + newReturnTypeName + "()";
            isPointer ? newName += ")" : newName;
            clang::SourceRange range;
            range.setBegin(E->getBeginLoc());
            range.setEnd(E->getEndLoc());
            if (range.isValid()) {
              std::string verifierString = "";
              llvm::raw_string_ostream tempStream(verifierString);
              verifierString += newName;
              llvm::outs() << verifierString << " - The String for RemoveVisitor\n";
              _Rewriter.ReplaceText(E->getSourceRange(), verifierString);
              llvm::outs() << "Inserted the text\n\n";
              // If the return type is not already in the _NeededTypes we add it here
              if (!_NeededTypes->count(E->getCallReturnType(*_C))) {
                _NeededTypes->emplace(E->getCallReturnType(*_C));
              }
            }
          }
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitCallExpr(E);
}

bool RemoveFuncVisitor::shouldTraversePostOrder() {
  // return true;
  return false;
}
