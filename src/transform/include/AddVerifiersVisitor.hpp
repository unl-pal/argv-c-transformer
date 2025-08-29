#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <set>

class AddVerifiersVisitor : public clang::RecursiveASTVisitor<AddVerifiersVisitor> {
public:
  /// Creates Verifier functions and writes to the output
  /// @param - c context of the tree used
  /// @param - output to be written to
  /// @param - neededTypes types of verifier functions needed to complete the code
  AddVerifiersVisitor(clang::ASTContext *c, llvm::raw_fd_ostream &output,
                      std::set<clang::QualType> *neededTypes,
                      clang::Rewriter &rewriter);

  /// Starts the traversal process
  virtual bool HandleTranslationUnit(clang::TranslationUnitDecl *D);

private:
  clang::ASTContext *_C;
  llvm::raw_fd_ostream &_Output;
  std::set<clang::QualType> *_NeededTypes;
  clang::Rewriter &_Rewriter;
};
