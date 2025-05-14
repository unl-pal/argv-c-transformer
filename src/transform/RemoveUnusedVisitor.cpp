#include "RemoveUnusedVisitor.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/Specifiers.h>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <unordered_set>
#include <vector>

RemoveUnusedVisitor::RemoveUnusedVisitor(clang::ASTContext *C)
: _C(C),
  _Mgr(_C->getSourceManager()),
  _ToRemove(std::vector<clang::Decl*>()),
  _TD(_C->getTranslationUnitDecl())
{
  // _ToRemove = std::unordered_set<clang::Decl*>();
}

bool RemoveUnusedVisitor::RemoveNodes(clang::TranslationUnitDecl *D) {
  if (!D) return false;

  llvm::outs() << "Past TD\n";
  llvm::outs() << "Removing " << _ToRemove.size() << " Nodes\n";
  int size = _ToRemove.size();
  for (int i = size-1; i>0; i--) {
    try {
      // llvm::outs() << i << "\n";
      clang::Decl *decl = _ToRemove[i];
      // llvm::outs() << _ToRemove.size() << "\n";
      // continue;
      // for (auto *child : decl->getDeclContext()->decls()) {
      //
      // }
      // if (decl != nullptr) {
      //   decl->dump();
      //       decl = NULL; // TODO this does nothing for me.... soooooo... yeah
      //       llvm::outs() << "I tried buddy\n";
      // }
      // _ToRemove[i] = nullptr;
    } catch (...) {
      continue;
    }
  }
  return true;
}

// bool RemoveUnusedVisitor::RemoveNodes(clang::Decl *D) {
//   if (!D) return false;
//
//   llvm::outs() << "Past TD\n";
//   llvm::outs() << "Removing " << _ToRemove.size() << " Nodes\n";
//   // clang::DeclContext *DC = (_C->getTranslationUnitDecl() == D) ? _C->getTranslationUnitDecl()->getDeclContext() : D->getDeclContext();
//   // bool yes = (_C->getTranslationUnitDecl() == D || (D->getDeclContext()));
//   // if  (yes) {
//   if (clang::DeclContext *DC = D->getDeclContext()) {
//     if (!DC->decls_empty()) {
//     auto decl = DC->decls_begin();
//       decl->dump();
//       while (decl != DC->decls_end()) {
//         if (_ToRemove.extract(*decl)) {
//           clang::Decl *temp = *decl;
//           decl = decl++;
//           temp->dump();
//           _ToRemove.erase(temp);
//           _TD->removeDecl(temp);
//         }
//         decl = decl++;
//       }
//     }
//     return false;
//   }
//   // return result;
//   return false;
// }

bool RemoveUnusedVisitor::VisitTranslationUnitDecl(clang::TranslationUnitDecl *TD) {
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitTranslationUnitDecl(TD);
}

bool RemoveUnusedVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
  // if (_Mgr.isInMainFile(D->getLocation())) {
  // if (!D->isUsed() || !_Mgr.isInMainFile(D->getLocation())) {
  // if (D->isUsed() || D->isReferenced()) {
  if (!D->isUsed() && !D->isReferenced()) {
        _ToRemove.push_back(D);
  // } else if (D->isDefinedOutsideFunctionOrMethod()) {
  // } else {
    // if (_C->getTranslationUnitDecl() != D) {
        // _ToRemove.push_back(D);
      // }
    // _ToRemove.push_back(D);
  }
  // if (!(D->isUsed()) && !_Mgr.isInExternCSystemHeader(D->getLocation())) {
  //   if (_C->getTranslationUnitDecl() != D) {
  //       _ToRemove.push_back(D);
  //     }
  //   }
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitDecl(D);
}

bool RemoveUnusedVisitor::VisitRecordDecl(clang::RecordDecl *D) {
  // if (D->getTypedefNameForAnonDecl()->
  if (D->field_empty()) {
    _ToRemove.push_back(D);
  }
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitRecordDecl(D);
}

bool RemoveUnusedVisitor::VisitTypedefDecl(clang::TypedefDecl *D) {
  // if (!D->isUsed() && !D->isReferenced()) {
  //   _ToRemove.push_back(D);
  // }
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitTypedefDecl(D);
}

bool RemoveUnusedVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl *D) {
  // if (!D->isUsed() && !D->isReferenced()) {
  //   _ToRemove.push_back(D);
  // }
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitTypedefNameDecl(D);
}

// bool RemoveUnusedVisitor::VisitExternCContextDecl(clang::ExternCContextDecl *D) {
//   if (!D) return false;
//   // if (!D->isUsed() || !_Mgr.isInMainFile(D->getLocation())) {
//   if (D->isUsed() || D->isReferenced()) {
//   } else if (D->isDefinedOutsideFunctionOrMethod()) {
//   } else {
//     _ToRemove.push_back(D);
//   }
//   // if (D->isDefinedOutsideFunctionOrMethod() && (D->isUsed() || D->isReferenced())) {
//     return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitExternCContextDecl(D);
//   // }
// }

bool RemoveUnusedVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  // if (D->isExternallyDeclarable()) {
    // llvm::outs() << "FromFile FuncDecl:\n";
        // _ToRemove.push_back(D);
  // if (!D->isUsed() || !_Mgr.isInMainFile(D->getLocation())) {
    // TODO should the ExternCHeader part be removed?
  if ((!D->isUsed() && !D->isReferenced()) || _Mgr.isInExternCSystemHeader(D->getLocation())) {
  // if (!D->isUsed() && !D->isReferenced()) {
      // D->dumpColor();
      // _C->getTranslationUnitDecl()->removeDecl(D);
      _ToRemove.push_back(D);
    // }
  }
  return clang::RecursiveASTVisitor<RemoveUnusedVisitor>::VisitFunctionDecl(D);
}
