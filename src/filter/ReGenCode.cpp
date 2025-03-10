#include "include/ReGenCode.h"
#include <clang-c/Index.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/CodeGenOptions.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

ReGenCodeVisitor::ReGenCodeVisitor(clang::ASTContext *C) : _C(C), _Mgr(_C->getSourceManager()) {
}

bool ReGenCodeVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
  /*if ((_C->getSourceManager().isInMainFile(D->getLocation()) && (D->hasBody() || D->isFromExplicitGlobalModule()) ) || (!_C->getSourceManager().isInMainFile(D->getLocation()) && D->isReferenced())) {*/
  bool yes = false;
  if (_Mgr.isInMainFile(D->getLocation())) {
    if (D->hasBody()) {
      yes = false;
    } else if (D->getDeclContext()->getParent() && D->getDeclContext()->isRecord()) {
      yes = false;
    } else if (!D->getDeclContext()->isTranslationUnit() && !D->getParentFunctionOrMethod()) {
      yes = true;
    }
  } else if (D->isUsed()) {
    yes = true;
  } else if (D->isReferenced()) {
    yes = true;
  }
  if (yes) {
    D->print(llvm::outs());
    llvm::outs() << ";\n";
  }

  /*if (D->hasBody()) {*/
  /*  D->print(llvm::outs());*/
  /*  llvm::outs() << "\n";*/
  /*}*/
  /*if (!D->getDeclContext()->isTranslationUnit()) {*/
  /*D->getDeclContext()->getDeclKind();*/
  /*if (D->isReferenced()) std::cout << "referenced" << std::endl;*/
  /*D->getDeclContext()->dumpAsDecl();*/
  /*}*/
  /*D->getDeclContext()->dumpDeclContext();*/
  /*}*/
  /*D->getDeclContext()->addDecl(Decl *D);*/
  /*D->getDeclContext()->isFunctionOrMethod();*/
  /*D->getDeclContext()->getParentASTContext().getCommentForDecl(const Decl *D, const Preprocessor *PP)*/
  return clang::RecursiveASTVisitor<ReGenCodeVisitor>::VisitDecl(D);
}

bool ReGenCodeVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (!D) return false;
  if (_Mgr.isInMainFile(D->getLocation())) {
    D->print(llvm::outs());
  }
  return clang::RecursiveASTVisitor<ReGenCodeVisitor>::VisitFunctionDecl(D);
}

bool ReGenCodeVisitor::VisitVarDecl(clang::VarDecl *D) {
  if (!D) return false;
  /*if (_Mgr.isInMainFile(D->getLocation()) && !D->getDeclContext()->getParent() && D->getDeclContext()->getParent()->getDeclKind() == clang::Decl::Record) {*/
  if (_Mgr.isInMainFile(D->getLocation()) && !D->getDeclContext()->getParent()) {
    D->print(llvm::outs());
    llvm::outs() << ";\n";
  }
  return clang::RecursiveASTVisitor<ReGenCodeVisitor>::VisitVarDecl(D);
}

bool ReGenCodeVisitor::VisitRecordDecl(clang::RecordDecl *D) {
  if (!D) return false;
  if (_Mgr.isInMainFile(D->getLocation())) {
    D->print(llvm::outs());
    llvm::outs() << ";\n";
  }
  /*return clang::RecursiveASTVisitor<ReGenCodeVisitor>::VisitRecordDecl(D);*/
  return true;
}

bool ReGenCodeVisitor::VisitFieldDecl(clang::FieldDecl *D) {
  if (!D) return false;
  /*&& D->getDeclContext()->getDeclKind() != clang::Decl::Field*/
  if (_Mgr.isInMainFile(D->getLocation())) {
  }
  /*return clang::RecursiveASTVisitor<ReGenCodeVisitor>::VisitFieldDecl(D);*/
  return true;
}




bool ReGenCodeVisitor::VisitStmt(clang::Stmt *S) {
  if (!S) return false;
  if (_Mgr.isInMainFile(S->getBeginLoc())) {
    /*S->printPretty(raw_ostream &OS, PrinterHelper *Helper, const PrintingPolicy &Policy)*/
    /*S->printPretty(llvm::outs(), PrinterHelper *Helper, const PrintingPolicy &Policy)*/
    /*S->getSourceRange().print(llvm::outs(), mgr);*/
    /*clang::PrinterHelper *helpME = clang::PrinterHelper(llvm::outs(), mgr);*/
    /*clang::PrintingPolicy policy = _C->getPrintingPolicy();*/
    /*if (S->getStmtClass() == clang::comments::FullComment) {*/
    /*if (S->getStmtClass() == clang::LineComment) {*/
      /*S->dumpColor();*/
    /*}*/
    /*clang::PrinterHelper *helpMe;*/
    /*helpMe->handledStmt(S, llvm::outs());*/
    /*S->printPretty(llvm::outs(), 0, policy, 0, "\n", _C);*/
    /*llvm::outs() << "\n";*/
  }
  return clang::RecursiveASTVisitor<ReGenCodeVisitor>::VisitStmt(S);
}
