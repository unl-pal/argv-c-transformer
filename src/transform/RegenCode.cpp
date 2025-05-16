#include "include/RegenCode.h"
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/raw_ostream.h>

RegenCodeVisitor::RegenCodeVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output)
    : _C(C),
  _M(C->getSourceManager()),
  _Output(output) {
}

bool RegenCodeVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitDecl(D);
}

bool RegenCodeVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (!D) return false;
    D->print(_Output);
  if (D->getAsFunction()->getStorageClass() == clang::SC_Extern) {
    _Output << ";";
  }
    _Output << "\n";
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitFunctionDecl(D);
}

bool RegenCodeVisitor::VisitVarDecl(clang::VarDecl *D) {
  if (!D) return false;
  if (!D->getDeclContext()->getParent()) {
    D->print(_Output);
    _Output << ";\n";
  }
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitVarDecl(D);
}

bool RegenCodeVisitor::VisitRecordDecl(clang::RecordDecl *D) {
  if (!D) return false;
  D->print(_Output);
  _Output << ";\n";
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitRecordDecl(D);
}

bool RegenCodeVisitor::VisitTypedefDecl(clang::TypedefDecl *D) {
  D->print(_Output);
  _Output << ";\n";
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitTypedefDecl(D);
}

bool RegenCodeVisitor::VisitUnnamedGlobalConstantDecl(
  clang::UnnamedGlobalConstantDecl *D) {
  D->print(_Output);
  _Output << ";\n";
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitUnnamedGlobalConstantDecl(D);
}
