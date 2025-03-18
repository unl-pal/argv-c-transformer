#include "include/Transform.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTDumperUtils.h>
#include <clang/AST/ASTImporter.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Sema/Ownership.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

TransformerVisitor::TransformerVisitor(clang::ASTContext *newC, clang::ASTContext *oldC, clang::Rewriter &R) :
  _NewC(newC),
  _OldC(oldC),
  _R(R),
  _M(&_NewC->getSourceManager()) {
}


bool TransformerVisitor::VisitTranslationUnitDecl(clang::TranslationUnitDecl *TD) {
  clang::TranslationUnitDecl *tempTd = _NewC->getTranslationUnitDecl();
  int size = VerifierFuncs.size();
  for (int i=0; i<size; i++) {
    clang::IdentifierInfo *funcName = &_NewC->Idents.get(VerifierFuncs[i]);
    clang::DeclarationName declName(funcName);
    clang::QualType returnType = ReturnTypes[i];
    clang::FunctionProtoType::ExtProtoInfo epi;
    clang::QualType funcQualType = _NewC->getFunctionType(returnType, clang::ArrayRef<clang::QualType>(), epi);
    clang::SourceLocation loc = tempTd->getLocation();

    clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
      *_NewC,
      TD,
      loc,
      loc,
      declName,
      funcQualType,
      nullptr,
      clang::SC_None
    );
    /*newFunction->setReferenced();*/
    tempTd->addDecl(newFunction);
  }
  for (clang::Decl *decl : TD->decls()) {
    tempTd->addDecl(decl);
  }
  TD = tempTd;
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitTranslationUnitDecl(tempTd);
}

bool TransformerVisitor::VisitDecl(clang::Decl *D) {
  if (D->isInvalidDecl()) {
  }
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitDecl(D);
}

bool TransformerVisitor::VisitStmt(clang::Stmt *S) {
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitStmt(S);
}

/// This is working for identifying function calls that are defined
bool TransformerVisitor::VisitDeclRefExpr(clang::DeclRefExpr *D) {
  if (!_NewC->getSourceManager().isInMainFile(D->getLocation())) return true;
  if (D->getType()->isFunctionType()) {
    if (D->getDecl()->isCanonicalDecl()) {
      if (clang::Decl *d = D->getDecl()) {
        if (clang::FunctionDecl *func = d->getAsFunction()) {
          if (!func->isDefined()) {
            std::string myType = func->getReturnType().getAsString();
            /*D->dumpColor();*/
            clang::IdentifierInfo *newInfo = &_NewC->Idents.get("__VERIFIER_nondet_"+myType);
            clang::DeclarationName newName(newInfo);
            func->setDeclName(newName);
            /*func->dumpColor();*/
            /*D->dumpColor();*/
            return true;
          }
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitDeclRefExpr(D);
}

/// I Think This Can Be Deleted...
/// Call Expr is the parent of the function decl ref and the args used
bool TransformerVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (!_OldC->getSourceManager().isInMainFile(E->getExprLoc())) return true;
  if (const auto *func = E->getCalleeDecl()->getAsFunction()) {
    if (!func->isImplicit()) return true;
    if (!func->isDefined()) {
      if (!E->arguments().empty()) {
        E->dumpColor();
        /*llvm::outs() << "Now What\n";*/
        E->shrinkNumArgs(0);
        E->computeDependence();
      }
    }
  }
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitCallExpr(E);
}
