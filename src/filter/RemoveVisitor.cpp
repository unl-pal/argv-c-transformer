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
                                     std::vector<std::string> toRemove,
                                     std::set<clang::QualType> *neededTypes)
    : _C(C),
      _mgr(rewriter.getSourceMgr()),
      _Rewriter(rewriter),
      _toRemove(toRemove),
      _NeededTypes(neededTypes) {}

bool RemoveFuncVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if(!D) return false;
  if (_mgr.isInMainFile(D->getLocation())) {
    for (std::string& name : _toRemove) {
      if (name == D->getNameAsString()) {
        // llvm::outs() << name << "\n";
        _NeededTypes->emplace(D->getReturnType());
        clang::SourceLocation zero = _mgr.translateLineCol(_mgr.getMainFileID(), _mgr.getSpellingLineNumber(D->getLocation()), 1);
        clang::SourceRange range = clang::SourceRange(zero, D->getEndLoc());
        // llvm::outs() << range.printToString(_mgr) << "\n";
        if (D->getStorageClass() == clang::SC_Extern) {
          range = clang::SourceRange(D->getOuterLocStart(), D->getEndLoc().getLocWithOffset(1));
        }
        _Rewriter.RemoveText(range);
        clang::RawComment *rawComment = _C->getRawCommentForDeclNoCache(D);
        if (rawComment != nullptr) {
          _Rewriter.RemoveText(rawComment->getSourceRange());
        }
      }
    }
  }
    return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
}

// TODO CallExpr can be used to also ID the return type for replacing with the
// correct versions of the verifier
bool RemoveFuncVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (!E) return false;
  if (_mgr.isInMainFile(E->getExprLoc())) {
    // TODO RETURN TYPE IS LOST BETWEEN FILTER AND TRANSFORM
    if (clang::FunctionDecl *func = E->getDirectCallee()) {
      std::string name = func->getNameAsString();
      for (std::string removedFuncName : _toRemove) {
        if (name == removedFuncName) {
          std::string newName = "";
          bool isPointer = E->getType()->isPointerType();
          // isPointer ?  newName += "new " : newName += "";
          std::string returnTypeName = E->getCallReturnType(*_C).getAsString();
          std::string newReturnTypeName = "";
          if (E->getCallReturnType(*_C)->isBooleanType()) {
          // if (returnTypeName == "_Bool") {
            newReturnTypeName = "bool";
          } else if (E->getCallReturnType(*_C)->isBuiltinType() || E->getCallReturnType(*_C)->isCharType() || E->getCallReturnType(*_C)->isAnyPointerType()) {
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
            // isPointer ? newName += "new " + newReturnTypeName : newName;
            isPointer ? newName += "(" + newReturnTypeName + "" : newName;
            // newName += newReturnTypeName;
            isPointer ? newName += "*)(" : newName += "";
            newName += "__VERIFIER_nondet_" + newReturnTypeName + "()";
            isPointer ? newName += ")" : newName;
            // std::replace(returnTypeName.begin(), returnTypeName.end(), ' ', '_');
            // auto done = returnTypeName.begin();
            // while (done != returnTypeName.end()) {
            //   done = std::remove(returnTypeName.begin(), returnTypeName.end(), '_');
            // }
            // done = returnTypeName.begin();
            // while (done != returnTypeName.end()) {
            //   done = std::remove(returnTypeName.begin(), returnTypeName.end(), '*');
            // }
            // newName += newReturnTypeName;
            // newName += "()";
            // llvm::outs() << E->getDirectCallee()->getName().str() << "\n";
            // clang::SourceRange range;
            // // range.setBegin((_mgr.getSpellingLineNumber(E->getBeginLoc()), E->getEndLoc().getLocWithOffset(-(name.size() - 1))));
            // range.setBegin(_mgr.translateLineCol(_mgr.getMainFileID(), _mgr.getSpellingLineNumber(E->getCallee()->getEndLoc()), _mgr.getSpellingColumnNumber(E->getCallee()->getBeginLoc())));
            // range.setEnd(E->getEndLoc());
            // range.dump(_mgr);
            // range = E->getSourceRange();
            // range.dump(_mgr);
            // E->getSourceRange().dump(_mgr);
            // _Rewriter.ReplaceText(range, newName);
            _Rewriter.ReplaceText(E->getSourceRange(), newName);
          } else {
            _Rewriter.RemoveText(E->getSourceRange());
          }
        }
      }
    }
    if (E->getType()->isFunctionType()) {
      auto thing = E->getCallReturnType(*_C);
      if (thing->isCharType()) {
      } else if (thing->isIntegerType()) {
      } else if (thing->isFloatingType()) {
      } else if (thing->isStructureType()) {
      } else if (thing->isObjectPointerType()) {
      } else if (thing->isArrayType()) {
      } else if (thing->isNullPtrType()) {
      // } else if (thing->isDoubleType()) {
      } else {
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitCallExpr(E);
}

bool RemoveFuncVisitor::shouldTraversePostOrder() {
  // return true;
  return false;
}
