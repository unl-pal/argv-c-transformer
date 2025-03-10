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
#include <iostream>
#include <vector>

RemoveFuncVisitor::RemoveFuncVisitor(clang::ASTContext *C, clang::Rewriter &R,
                                     std::vector<std::string> toRemove)
    : _C(C), _R(R), _mgr(_R.getSourceMgr()), _toRemove(toRemove) {}

bool RemoveFuncVisitor::VisitStmt(clang::Stmt *S) {
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitStmt(S);
}

bool RemoveFuncVisitor::VisitTranslationUnitDecl(clang::TranslationUnitDecl *D) {
  if (D) {
    /*clang::RawComment *hello = _C->getSourceManager().getloc"---------------------------------\n"*/
    /*    "!! This File Has Been Modified !!\n"*/
    /*    "---------------------------------\n";*/
    /*_C->addComment(hello);*/
    /*auto loc = _C->getSourceManager().getIncludeLoc(_C->getSourceManager().getMainFileID());*/
    /*auto loc = D->getLocation();*/
    /*_R.InsertTextAfterToken(loc, "Hello");*/
  }
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitDecl(D);
}

bool RemoveFuncVisitor::VisitDecl(clang::Decl *D) {
  if (D->isCanonicalDecl()) {
    /*clang::RawComment *hello = _C->getSourceManager().getloc"---------------------------------\n"*/
    /*    "!! This File Has Been Modified !!\n"*/
    /*    "---------------------------------\n";*/
    /*_C->addComment(hello);*/
    /*auto loc = _C->getSourceManager().getIncludeLoc(_C->getSourceManager().getFileID(D->getLocation()));*/
    /*_R.InsertTextBefore(loc, "Hello");*/
  }
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitDecl(D);
}

bool RemoveFuncVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if(!D) return false;
  if (_mgr.isInMainFile(D->getLocation())) {
    std::cout << "In Main File" << std::endl;
    for (std::string& name : _toRemove) {
      if (name == D->getNameAsString()) {
          std::cout << "Matched Name" << std::endl;
        // TODO replace all references to the function with a 'type' ref
        /*clang::QualType type = D->getReturnType();*/
        /*D->getDeclContext()->getParent()->removeDecl(D);*/
        /*std::cout << "Removed Node" << std::endl;*/
        /*return false;*/
        if (clang::TranslationUnitDecl *TU =
                clang::dyn_cast<clang::TranslationUnitDecl>(
                    D->getDeclContext())) {
          auto thing = D->getSourceRange();
          _R.RemoveText(thing);
          /*D->setDeletedAsWritten();*/
          TU->removeDecl(D);
          std::cout << "Removed Node" << std::endl;
          return false;
        }
      }
    }
    return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
  }
  return true;
}

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
      /*} else if (thing->is) {*/
      /*} else if (thing->) {*/
      /*} else if (thing->) {*/
      } else {
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitCallExpr(E);
}
