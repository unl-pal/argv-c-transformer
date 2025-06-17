#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <set>

class CreateVerifiersVisitor : public clang::RecursiveASTVisitor<CreateVerifiersVisitor> {
public:
  CreateVerifiersVisitor(clang::ASTContext *c, llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes);

  virtual bool HandleTranslationUnit(clang::TranslationUnitDecl *D);

private:
  clang::ASTContext *_C;
  llvm::raw_fd_ostream &_Output;
  std::set<clang::QualType> *_NeededTypes;
};
