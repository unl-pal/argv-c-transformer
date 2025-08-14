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
#include <llvm20/include/clang/AST/Expr.h>
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
      if (D->getLocation().isValid() && D->getLocation().isMacroID()) {
        llvm::outs() << name << " is macro thingy" << "\n";
        return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
      }
      if (name == D->getNameAsString() && !D->isMain()) {
        clang::SourceLocation zero = _mgr.translateLineCol(_mgr.getMainFileID(), _mgr.getSpellingLineNumber(D->getLocation()), 1);
        clang::SourceRange range = clang::SourceRange(zero, D->getEndLoc());
        // if (D->getStorageClass() == clang::SC_Extern || D->isStatic()) {
        if (!D->hasBody()) {
          range.setEnd(D->getEndLoc().getLocWithOffset(1));
        }
        if (range.isValid()) {
          llvm::outs() << name << " Is Valid Range" << "\n";
          _Rewriter.RemoveText(range);
          llvm::outs() << name << " Has Been Removed" << "\n";
        }
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
    if (clang::FunctionDecl *func = E->getDirectCallee()) {
      std::string name = func->getNameAsString();
      llvm::outs() << name << " call is being checked" << "\n";
      clang::QualType returnType = E->getCallReturnType(*_C);
      for (std::string removedFuncName : *_toRemove) {
        if (name == removedFuncName) {
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
