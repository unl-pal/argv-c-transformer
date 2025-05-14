#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <unordered_set>
#include <vector>
class RemoveUnusedVisitor : public clang::RecursiveASTVisitor<RemoveUnusedVisitor> {
public:
  RemoveUnusedVisitor(clang::ASTContext *C);

  bool VisitDecl(clang::Decl *D);

  bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *TD);

  // bool VisitExternCContextDecl(clang::ExternCContextDecl *D);

  bool VisitFunctionDecl(clang::FunctionDecl *D);

  bool RemoveNodes(clang::TranslationUnitDecl *D);

  bool VisitRecordDecl(clang::RecordDecl *D);

  bool VisitTypedefDecl(clang::TypedefDecl *D);

  bool VisitTypedefNameDecl(clang::TypedefNameDecl *D);

  // bool RemoveNodes(clang::Decl *D);

private:
  clang::ASTContext *_C;
  clang::SourceManager &_Mgr;
  std::vector<clang::Decl*> _ToRemove;
  clang::TranslationUnitDecl *_TD;

};
