#include "include/Remove.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <unordered_map>

RemoveFuncVisitor::RemoveFuncVisitor(clang::ASTContext *C, clang::Rewriter &R, std::unordered_map<std::string, bool> toRemove)
    : _C(C), _R(R), _mgr(_R.getSourceMgr()), _toRemove(toRemove) {
}

bool RemoveFuncVisitor::VisitStmt(clang::Stmt *S) {
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitStmt(S);
}

bool RemoveFuncVisitor::VisitDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitDecl(D);
}

bool RemoveFuncVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if(!D) return false;
  if (_mgr.isInMainFile(D->getLocation())) {
    if (_toRemove[D->getNameAsString()]) {
      // TODO replace all references to the function with a 'type' ref
      /*clang::QualType type = D->getReturnType();*/
      D->getDeclContext()->getParent()->removeDecl(D);
      return false;
    }
  }
  return true;
}
