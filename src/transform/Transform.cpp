#include "include/Transform.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>

TransformerVisitor::TransformerVisitor(clang::ASTContext *C, clang::Rewriter &R) :
  _C(C),
  _R(R),
  _M(&_C->getSourceManager()) {
}


bool TransformerVisitor::VisitDecl(clang::Decl *D) {
  if (D->isInvalidDecl()) {
    /*D->dumpColor();*/
  }
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitDecl(D);
}

bool TransformerVisitor::VisitStmt(clang::Stmt *S) {
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitStmt(S);
}

/// This is working for identifying function calls that are defined
bool TransformerVisitor::VisitDeclRefExpr(clang::DeclRefExpr *D) {
  if (!_C->getSourceManager().isInMainFile(D->getLocation())) return true;
  if (D->getType()->isFunctionType()) {
    if (D->getDecl()->isCanonicalDecl()) {
      if (auto *what = D->getDecl()) {
        /*what->dumpColor();*/
        if (auto *func = what->getAsFunction()) {
          if (func->isDefined()) {
            D->dumpColor();
          }
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitDeclRefExpr(D);
}

/// None of these are triggering on function call with undefined function
bool TransformerVisitor::VisitCallExpr(clang::CallExpr *E) {
  /*E->dumpColor();*/
  if (!_C->getSourceManager().isInMainFile(E->getExprLoc())) return true;
  /*E->dumpColor();*/
  /*E->*/

  /*clang::CallExpr::Create(const ASTContext &Ctx, Expr *Fn, ArrayRef<Expr *> Args, QualType Ty, ExprValueKind VK, SourceLocation RParenLoc, FPOptionsOverride FPFeatures)*/
  if (E->getReferencedDeclOfCallee()) {
    E->getCalleeDecl()->getDeclContext()->dumpAsDecl();
  }
  if (E->getCallee()->getType()->isFunctionType()) {
    if (E->getCalleeDecl()->isInvalidDecl()) {
    }
    clang::FunctionDecl *FD = E->getCalleeDecl()->getAsFunction();
    if (!FD->isDefined()) {
      if (clang::DeclContext *P = FD->getDeclContext()->getParent()) {
        std::string returnType = FD->getReturnType().getAsString();
        _R.RemoveText(FD->getSourceRange());
        _R.InsertText(FD->getLocation(), "VERIFIER_FOR" + returnType);
        P->removeDecl(FD);
        /// TODO This will most likely break everything
        clang::Decl *N(nullptr);
        N->setLocation(FD->getLocation());
        N->setDeclContext(FD->getDeclContext());
        P->addDecl(N);
        return true;
      }
    }
  }
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitCallExpr(E);
}
