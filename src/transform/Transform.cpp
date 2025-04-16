#include "include/Transform.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTDumperUtils.h>
#include <clang/AST/ASTImporter.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/AST/NestedNameSpecifier.h>
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
      clang::SC_Extern
    );
    newFunction->setReferenced();
    newFunction->setIsUsed();
    tempTd->addDecl(newFunction);
    _R.InsertTextBefore(tempTd->getBeginLoc(), declName.getAsString());
  }
  for (clang::Decl *decl : TD->decls()) {
    tempTd->addDecl(decl);
    _R.InsertTextBefore(TD->getLocation(), decl->getSourceRange().printToString(_NewC->getSourceManager()));
  }
  // TD = tempTd;
  // return clang::RecursiveASTVisitor<TransformerVisitor>::VisitTranslationUnitDecl(TD);
  if (_OldC->Comments.empty()) {
    for (auto comment : *_OldC->Comments.getCommentsInFile(_M->getFileID(TD->getLocation()))) {
      if (!comment.second->isAttached()) {
        llvm::outs() << "Comment is not Attached\n";
        _R.ReplaceText(comment.second->getSourceRange(), "");
      }
    }
  }
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

// This is working for identifying function calls that are defined
bool TransformerVisitor::VisitDeclRefExpr(clang::DeclRefExpr *D) {
  if (!_OldC->getSourceManager().isInMainFile(D->getLocation())) return true;
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitDeclRefExpr(D);
}

// I Think This Can Be Deleted...
// Call Expr is the parent of the function decl ref and the args used
bool TransformerVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (!_OldC->getSourceManager().isInMainFile(E->getExprLoc())) return true;
  if (clang::FunctionDecl *func = E->getCalleeDecl()->getAsFunction()) {
    if ((func->isImplicit()) || (!func->isDefined() && !func->isExternC())) {
      std::string myType = func->getReturnType().getAsString();
      clang::IdentifierInfo *newInfo = &_NewC->Idents.get("__VERIFIER_nondet_"+myType);
      clang::DeclarationName newName(newInfo);
      func->setDeclName(newName);
      E->shrinkNumArgs(0);
      _R.ReplaceText(E->getSourceRange(), newName.getAsString() + "()");
    }
  }
  return clang::RecursiveASTVisitor<TransformerVisitor>::VisitCallExpr(E);
}
