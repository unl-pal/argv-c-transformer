#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

class CreateVerifiersVisitor : public clang::RecursiveASTVisitor<CreateVerifiersVisitor> {
public:
  CreateVerifiersVisitor(clang::ASTContext *c, clang::SourceManagerForFile &smf,
                         llvm::raw_fd_ostream &output);

  virtual bool HandleTranslationUnit(clang::TranslationUnitDecl *D);

private:
  clang::ASTContext *_C;
  clang::SourceManagerForFile &_SMF;
  llvm::raw_fd_ostream &_Output;
};
