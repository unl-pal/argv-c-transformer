#include "include/RegenCode.h"
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/raw_ostream.h>

RegenCodeVisitor::RegenCodeVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output)
    : _C(C),
  _M(C->getSourceManager()),
  _Output(output) {
  // if (!_C->Comments.empty()) {
  //   _Comments = _C->DeclRawComments;
  // }
}

bool RegenCodeVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
  if (!_C->Comments.empty()) {
    if (auto mapOComments = _C->Comments.getCommentsInFile(_M.getMainFileID())) {
      // _Output << mapOComments->begin()->second->getRawText(_M) << "\n";
      // if (D->getID()) {
        // if (mapOComments->find(D->getID()) != mapOComments->end()) {
          // _Output << mapOComments->find(D->getID())->second->getRawText(_M);
          // auto iteratorForComment = _C->DeclRawComments.find(D);
          // _Output << iteratorForComment->second->getRawText(_M) << "\n";
        // }
      // }
      // const clang::RawComment *comment = _C->DeclRawComments.lookup(D);
      // _Output << _M.getCharacterData();
      // llvm::outs() << "Comments are recognized but not printed";
      // _Output << comment->getSourceRange().printToString(_M);
      // _Output << comment->getBriefText(*_C);
    }
  }
  auto denseComments = _C->Comments.getCommentsInFile(_M.getMainFileID());
  if (!denseComments->empty()) {
    auto i = denseComments->begin();
    for (const auto i : *_C->Comments.getCommentsInFile(_M.getMainFileID())) {
      _Output << i.second->getRawText(_M) << "\n";
      // _Output << i->second->getRawText(_M) << "\n";
      // i++;
    }
    // llvm::outs() << "Comments are recognized but not printed";
  }
  // D->print(_Output);
  // _Output << "===============;\n";
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitDecl(D);
}

bool RegenCodeVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (!D) return false;
  if (_C->DeclRawComments.find(D) != _C->DeclRawComments.end()) {
    const clang::RawComment *comment = _C->DeclRawComments.at(D);
    _Output << comment->getBriefText(*_C);
  }
    D->print(_Output);
  if (D->getAsFunction()->getStorageClass() == clang::SC_Extern) {
    _Output << ";";
  }
    _Output << "\n";
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitFunctionDecl(D);
}

bool RegenCodeVisitor::VisitVarDecl(clang::VarDecl *D) {
  if (!D) return false;
  if (_C->DeclRawComments.find(D) != _C->DeclRawComments.end()) {
    const clang::RawComment *comment = _C->DeclRawComments.at(D);
    _Output << comment->getRawText(_M);
  }
  if (!D->getDeclContext()->getParent()) {
    D->print(_Output);
    _Output << ";\n";
  }
  return clang::RecursiveASTVisitor<RegenCodeVisitor>::VisitVarDecl(D);
}

bool RegenCodeVisitor::VisitRecordDecl(clang::RecordDecl *D) {
  if (!D) return false;
  if (_C->DeclRawComments.find(D) != _C->DeclRawComments.end()) {
    const clang::RawComment *comment = _C->DeclRawComments.at(D);
    _Output << comment->getRawText(_M);
  }
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
