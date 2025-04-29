#include "include/ReGenCode.h"
#include <clang-c/Index.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/CodeGenOptions.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/raw_ostream.h>

ReGenCodeVisitor::ReGenCodeVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output)
    : _C(C),
  _Mgr(_C->getSourceManager()),
  _Output(output) {}

bool ReGenCodeVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
  bool yes = false;
  if (_Mgr.isInMainFile(D->getLocation())) {
    if (D->hasBody()) {
      yes = false;
    } else if (D->getDeclContext()->getParent() &&
               D->getDeclContext()->isRecord()) {
      yes = false;
    } else if (!D->getDeclContext()->isTranslationUnit() &&
               !D->getParentFunctionOrMethod()) {
      yes = true;
    }
  } else if (D->isFunctionOrFunctionTemplate() || D->getKind() == D->Typedef) {
    if (D->isUsed()) {
      yes = true;
    // } else if (D->isReferenced()) {
      // yes = true;
      // } else if (D->isReferenced()) {
      // yes = true;
    /*} else if (D->Typedef) {*/
    }
  }
  // if (clang::FunctionDecl *func = D->getAsFunction()) {
  //   if (func->getFirstDecl() != func) {
  //   yes = false;
  //   }
  // }
  if (yes) {
    D->print(_Output);
    _Output << ";\n";
  }
  return clang::RecursiveASTVisitor<ReGenCodeVisitor>::VisitDecl(D);
}

bool ReGenCodeVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (!D) return false;
  if (_Mgr.isInMainFile(D->getLocation())) {
    if (D->isLocalExternDecl()) {
      D->print(_Output);
      _Output << ";";
    }
    D->print(_Output);
    _Output << "\n";
  }
  return clang::RecursiveASTVisitor<ReGenCodeVisitor>::VisitFunctionDecl(D);
}

bool ReGenCodeVisitor::VisitVarDecl(clang::VarDecl *D) {
  if (!D) return false;
  if (_Mgr.isInMainFile(D->getLocation()) &&
      !D->getDeclContext()->getParent()) {
    D->print(_Output);
    _Output << ";\n";
  }
  return clang::RecursiveASTVisitor<ReGenCodeVisitor>::VisitVarDecl(D);
}

bool ReGenCodeVisitor::VisitRecordDecl(clang::RecordDecl *D) {
  if (!D) return false;
  if (_Mgr.isInMainFile(D->getLocation())) {
      D->print(_Output);
    _Output << ";\n";
  }
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

 
