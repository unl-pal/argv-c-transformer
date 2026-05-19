#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>
#include <vector>

/// Visitor for adding a Main function to a potential benchmark
class AddMainVisitor : public clang::RecursiveASTVisitor<AddMainVisitor> {
public:
  /// Visitor for adding a Main function to a potential benchmark
  /// @param - c context for the ast
  /// @param - functionsToCall vector of all functions that are not called or
  /// referenced at somepoint in the code
  /// @param - r Rewriter for writing to source file
  AddMainVisitor(clang::ASTContext *c, std::vector<std::string> functionsToCall,
                 clang::Rewriter &r);

  /// Begins Visitor traversing on the AST
  /// @param D translation unit for the AST
  bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *D);

private:
  clang::ASTContext *_Context;
  std::vector<std::string> &_FunctionsToCall;
  clang::Rewriter &_Rewriter;

};
