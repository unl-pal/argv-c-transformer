#include "RemoveUnusedVisitor.hpp"
#include <llvm/Support/raw_ostream.h>

/// Recursively Remove Nodes
/// At this time due to the nature of the visitor only ONE node is removed during
/// each visit/run. Due to the Depth first nature, this is not the most efficient
/// and should be looked into.
/// Returns a False on each node deleted to inform the user that removal is not
/// done. When no nodes are left to remove the True result is returned

RemoveUnusedVisitor::RemoveUnusedVisitor(clang::ASTContext *C)
: _C(C),
  _Mgr(_C->getSourceManager()),
  _TD(_C->getTranslationUnitDecl())
{
}

bool RemoveUnusedVisitor::VisitTranslationUnitDecl(clang::TranslationUnitDecl *TD) {
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitTranslationUnitDecl(TD);
}

bool RemoveUnusedVisitor::VisitRecordDecl(clang::RecordDecl *D) {
  if (!D->isUsed() && !D->isReferenced()) {
    if (!D->isAnonymousStructOrUnion()) {
      // D->dumpColor();
      _TD->removeDecl(D);
      return false;
    }
  }
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitRecordDecl(D);
}

bool RemoveUnusedVisitor::VisitTypedefDecl(clang::TypedefDecl *D) {
  if (!D->isUsed() && !D->isReferenced()) {
    // D->dumpColor();
    _TD->removeDecl(D);
      return false;
  }
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitTypedefDecl(D);
}

bool RemoveUnusedVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl *D) {
  if (!D->isUsed() && !D->isReferenced()) {
    // D->dumpColor();
    _TD->removeDecl(D);
      return false;
  }
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitTypedefNameDecl(D);
}

bool RemoveUnusedVisitor::VisitExternCContextDecl(clang::ExternCContextDecl *D) {
  if (!D->isUsed() && !D->isReferenced()) {
    D->getParent()->removeDecl(D);
    return false;
  }
    return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitExternCContextDecl(D);
}

bool RemoveUnusedVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (D->getNameAsString() == "main") {
    return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitFunctionDecl(D);
  }
  if (!D->isUsed() && !D->isReferenced()) {
    D->getParent()->removeDecl(D);
    return false;
  }
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitFunctionDecl(D);
}
