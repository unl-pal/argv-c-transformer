#include "include/RemoveVisitor.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm20/include/llvm/Support/raw_ostream.h>
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
      _NeededTypes(neededTypes) {}

bool RemoveFuncVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if(!D) return false;
  if (_mgr.isInMainFile(D->getLocation())) {
    llvm::outs() << D->getNameAsString() << " is being checked" << "\n";
    for (std::string& name : *_toRemove) {
      if (D->getLocation().isValid() && D->getLocation().isMacroID()) {
        llvm::outs() << name << " is macro thingy" << "\n";
        return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
      }
      if (name == D->getNameAsString()) {
        clang::SourceLocation zero = _mgr.translateLineCol(_mgr.getMainFileID(), _mgr.getSpellingLineNumber(D->getLocation()), 1);
        clang::SourceRange range = clang::SourceRange(zero, D->getEndLoc());
        if (D->getStorageClass() == clang::SC_Extern) {
          range.setEnd(D->getEndLoc().getLocWithOffset(1));
        }
        if (range.isValid()) {
          llvm::outs() << name << " Is Valid Range" << "\n";
          _Rewriter.RemoveText(range);
        }
        clang::RawComment *rawComment = _C->getRawCommentForDeclNoCache(D);
        if (rawComment != nullptr) {
          _Rewriter.RemoveText(rawComment->getSourceRange());
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
}

bool RemoveFuncVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (!E) return false;
  if (_mgr.isInMainFile(E->getExprLoc())) {
    if (clang::FunctionDecl *func = E->getDirectCallee()) {
      std::string name = func->getNameAsString();
      llvm::outs() << name << " call is being checked" << "\n";

      for (std::string removedFuncName : *_toRemove) {
        if (name == removedFuncName) {
          if (func->getLocation().isValid() && func->getLocation().isMacroID()) {
            llvm::outs() << name << " is macro thingy" << "\n";
            return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(func);
          }
          if (!_NeededTypes->count(E->getCallReturnType(*_C))) {
            _NeededTypes->emplace(E->getCallReturnType(*_C));
          }
          clang::QualType callReturnType = E->getCallReturnType(*_C);
          std::string newName = "";
          bool isPointer = callReturnType->isPointerType();
          bool isBool = callReturnType->isBooleanType();
          std::string returnTypeName = callReturnType.getAsString();
          std::string newReturnTypeName = "";
          if (isBool) {
            newReturnTypeName = "bool";
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
            isPointer ? newName += "(" + newReturnTypeName + "" : newName;
            isPointer ? newName += "*)(" : newName += "";
            newName += "__VERIFIER_nondet_" + newReturnTypeName + "()";
            isPointer ? newName += ")" : newName;
            clang::SourceRange range;
            range.setBegin(_mgr.translateLineCol(_mgr.getMainFileID(), _mgr.getSpellingLineNumber(E->getCallee()->getEndLoc()), _mgr.getSpellingColumnNumber(E->getCallee()->getBeginLoc())));
            range.setEnd(E->getRParenLoc());
            // DEBUG STUFF
            // if (range.isValid()) {
            // llvm::outs() << name << " Range is Valid" << "\n";
            // range.dump(_mgr);
            // llvm::outs() << newName << "\n";
            // llvm::outs() << _Rewriter.isRewritable(E->getCallee()->getExprLoc()) << "\n";
            // if (auto thing = _mgr.getCharacterData(E->getCallee()->getExprLoc())) {
            // llvm::outs() << thing << "\n";
            llvm::outs() << _Rewriter.ReplaceText(range, newName);
            // llvm::outs() << name << "Replaced Text" << "\n";
            // }
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
