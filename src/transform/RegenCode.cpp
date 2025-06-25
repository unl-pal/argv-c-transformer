#include "include/RegenCode.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/Support/raw_ostream.h>

// param *C a pointer to a Context
// param &output is an output stream to terminal, file, etc.
// class var _M is a SourceManager derived from the provided context
//    _M can be used to find info on location which can be used for Rewriters and other uses
RegenCodeVisitor::RegenCodeVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output)
    : _C(C),
  _M(C->getSourceManager()),
  _Output(output) {
}

// Catch all do nothing unless specified
bool RegenCodeVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
  if (!D->getParentFunctionOrMethod()) {
    if (clang::RawComment * rawComment = _C->getRawCommentForDeclNoCache(D)) {
      _Output << rawComment->getRawText(_M) << "\n";
    }
  }
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitDecl(D);
}

// Prints functions and their children with a ';' for externs
bool RegenCodeVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (!D) return false;
  if (!_M.isInMainFile(D->getLocation())) return true;
  if (D->getAsFunction()->getStorageClass() == clang::SC_Extern) {
    if (!D->getName().starts_with("__VERIFIER_nondet_")) {
      D->print(_Output);
      _Output << ";\n";
    }
  } else {
    D->print(_Output);
    // _Output << "\n";
  }
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitFunctionDecl(D);
}

// Print globally avaiable variables not parameters or function specific
bool RegenCodeVisitor::VisitVarDecl(clang::VarDecl *D) {
  if (!D) return false;
  if (!_M.isInMainFile(D->getLocation())) return true;
  if (D->isDefinedOutsideFunctionOrMethod()) {
    if (!D->isLocalVarDeclOrParm()) {
      D->print(_Output);
      _Output << ";\n";
    }
  }
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitVarDecl(D);
}

// Print Structs and Unions and their children
bool RegenCodeVisitor::VisitRecordDecl(clang::RecordDecl *D) {
  if (!D) return false;
  if (!_M.isInMainFile(D->getLocation())) return true;
  if (!D->isAnonymousStructOrUnion()) {
      D->print(_Output);
      _Output << ";\n";
  }
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitRecordDecl(D);
}

// Print TypeDefs
bool RegenCodeVisitor::VisitTypedefDecl(clang::TypedefDecl *D) {
  if (!_M.isInMainFile(D->getLocation())) return true;
      D->print(_Output);
      _Output << ";\n";
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitTypedefDecl(D);
}

// Parameter Variables are handled in the functions so they are passed over via this function
bool RegenCodeVisitor::VisitParmVarDecl(clang::ParmVarDecl * D){
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitParmVarDecl(D);
}

// Field Declarations are handled by the Struct and Union print and are passed over
bool RegenCodeVisitor::VisitFieldDecl(clang::FieldDecl * D){
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitFieldDecl(D);
}

// Print Unnamed Global Constants
bool RegenCodeVisitor::VisitUnnamedGlobalConstantDecl(
  clang::UnnamedGlobalConstantDecl *D) {
  if (!_M.isInMainFile(D->getLocation())) return true;
  D->print(_Output);
  _Output << ";\n";
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitUnnamedGlobalConstantDecl(D);
}

bool RegenCodeVisitor::shouldTraversePostOrder() {
  return false;
}
