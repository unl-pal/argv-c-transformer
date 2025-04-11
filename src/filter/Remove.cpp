#include "include/Remove.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
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
        // TODO replace all references to the function with a 'type' ref
        /*clang::QualType type = D->getReturnType();*/
        /*D->getDeclContext()->getParent()->removeDecl(D);*/
        /*std::cout << "Removed Node" << std::endl;*/
        /*return false;*/
        /// TODO check if this works with the tree regen
        /// keeps function def
        /// loses function body
        // _R.InsertTextBefore(D->getBeginLoc(), "// ====== Try Again Little Remover ======\n");
        // D->setBody(nullptr);
        /// TODO remove this?? VV
        // if (clang::TranslationUnitDecl *TU =
        //         clang::dyn_cast<clang::TranslationUnitDecl>(
        //             D->getDeclContext())) {
        //   // TU->removeDecl(D);
        // /// TODO remove this?? ^^
        //   // return false;
        // }
        _R.ReplaceText( D->getSourceRange(), "// === Once More Little Remove ===\n");
        return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
      }
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
