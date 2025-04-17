#include "include/Remove.h"

#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Support/raw_ostream.h>
#include <vector>

RemoveFuncVisitor::RemoveFuncVisitor(clang::ASTContext *C, clang::Rewriter &R,
                                     std::vector<std::string> toRemove)
  : _C(C), _R(R), _mgr(_R.getSourceMgr()), _toRemove(toRemove) {}

bool RemoveFuncVisitor::VisitStmt(clang::Stmt *S) {
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitStmt(S);
}

bool RemoveFuncVisitor::VisitTranslationUnitDecl(clang::TranslationUnitDecl *D) {
  if (!D) return true;
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitTranslationUnitDecl(D);
}

bool RemoveFuncVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitDecl(D);
}

bool RemoveFuncVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if(!D) return false;
  if (_mgr.isInMainFile(D->getLocation())) {
    for (std::string& name : _toRemove) {
      if (name == D->getNameAsString()) {
        llvm::outs() << "Removing Node: " << D->getNameAsString() << "\n";
        llvm::outs() << "Removed Node Code\n";
        if (_C->Comments.empty()) {
          llvm::outs() << "There are NO comments in file\n";
        }
        if (_C->DeclRawComments.count(D)) {
          clang::SourceRange commentRange = _C->DeclRawComments.find(D)->second->getSourceRange();
          _R.ReplaceText(commentRange, "");
        }
        if (clang::RawComment *rawComment = _C->getRawCommentForDeclNoCache(D)) {
          _R.ReplaceText(rawComment->getSourceRange(), "");
          llvm::outs() << "Handled RawComment\n";
        }
        _R.ReplaceText( D->getSourceRange(), "// === Removed Undesired Function ===\n");
        return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
      }
        // D->setBody(nullptr);
        /// TODO remove Node or Just Text?? VV
        // if (clang::TranslationUnitDecl *TU =
        //         clang::dyn_cast<clang::TranslationUnitDecl>(
        //             D->getDeclContext())) {
        //   // TU->removeDecl(D);
        // /// TODO remove this?? ^^
        //   // return false;
        // }
    }
    // _R.InsertTextBefore(D->getSourceRange().getBegin(), "// Keeping Function: \n");
    // return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
  }
    return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
}

// TODO Remove this for now?
bool RemoveFuncVisitor::VisitCallExpr(clang::CallExpr *E) {
  /*if (E->EvaluateAsBooleanCondition(bool &Result, const ASTContext &Ctx)) {*/
  if (!E) return false;
  if (_mgr.isInMainFile(E->getExprLoc())) {
    if (E->getType()->isFunctionType()) {
      auto thing = E->getCallReturnType(*_C);
      if (thing->isCharType()) {
      } else if (thing->isIntegerType()) {
      } else if (thing->isFloatingType()) {
      } else if (thing->isStructureType()) {
      } else if (thing->isObjectPointerType()) {
      } else if (thing->isArrayType()) {
      } else if (thing->isNullPtrType()) {
      } else if (thing->isDoubleType()) {
      } else {
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitCallExpr(E);
}
