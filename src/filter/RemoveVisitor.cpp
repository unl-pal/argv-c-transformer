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

RemoveFuncVisitor::RemoveFuncVisitor(clang::ASTContext       *C,
                                     clang::Rewriter &rewriter,
                                     std::vector<std::string> toRemove)
    : _C(C), _mgr(rewriter.getSourceMgr()), _Rewriter(rewriter), _toRemove(toRemove) {
}

bool RemoveFuncVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if(!D) return false;
  if (_mgr.isInMainFile(D->getLocation())) {
    for (std::string& name : _toRemove) {
      if (name == D->getNameAsString()) {
        llvm::outs() << name << "\n";
        // clang::SourceRange range = D->getSourceRange();
        clang::SourceLocation zero = _mgr.translateLineCol(_mgr.getMainFileID(), _mgr.getSpellingLineNumber(D->getLocation()), 1);
        clang::SourceRange range = clang::SourceRange(zero, D->getEndLoc());
        llvm::outs() << range.printToString(_mgr) << "\n";
        if (D->getStorageClass() == clang::SC_Extern) {
          range = clang::SourceRange(D->getOuterLocStart(), D->getEndLoc().getLocWithOffset(1));
        }
        _Rewriter.RemoveText(range);
        // _Rewriter.ReplaceText(range, "");
        clang::RawComment *rawComment = _C->getRawCommentForDeclNoCache(D);
        if (rawComment != nullptr) {
          // _Rewriter.ReplaceText(rawComment->getSourceRange(), "");
          _Rewriter.RemoveText(rawComment->getSourceRange());
        }
        // _Rewriter.ReplaceText(range, "// === Removed Undesired Function ===\n");
        // if (_C->getTranslationUnitDecl()->containsDecl(D)) {
        //   _C->getTranslationUnitDecl()->removeDecl(D);
        // } else {
        //   clang::IdentifierInfo *newInfo = &_C->Idents.get("0func" + name);
        //   clang::DeclarationName newName(newInfo);
        //   D->setDeclName(newName);
        // }
        // return false;
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
          // std::string returnType = E->getCall
          _Rewriter.ReplaceText(E->getSourceRange(), "__VERIFIER_nondet_" + func->getReturnType().getAsString() + "()");
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
