#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclID.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <sys/types.h>
#include <vector>

class RemoveUnusedVisitor : public clang::RecursiveASTVisitor<RemoveUnusedVisitor> {
public:
  RemoveUnusedVisitor(clang::ASTContext *C);

  // bool VisitDecl(clang::Decl *D);

  bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *TD);

  bool VisitExternCContextDecl(clang::ExternCContextDecl *D);

  bool VisitFunctionDecl(clang::FunctionDecl *D);

  bool RemoveNodes(clang::TranslationUnitDecl *D);
  // bool RemoveNodes();
  // bool RemoveNodes(clang::ASTContext *newContext);
  // bool RemoveNodes(clang::Decl *D);
  bool VisitTypedefNameDecl(clang::TypedefNameDecl *D);
  bool VisitTypedefDecl(clang::TypedefDecl *D);
  bool VisitRecordDecl(clang::RecordDecl *D);

  void Update(clang::ASTContext *C);

  int ReportRemoves();

private:
  clang::ASTContext *_C;
  clang::SourceManager &_Mgr;
  // std::unordered_set<clang::Decl*> _ToRemove;
  // std::unordered_map<ulong, clang::Decl*> _ToRemove;
  std::vector<clang::Decl*> _ToRemove;
  clang::TranslationUnitDecl *_TD;

};
