#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <set>

class ReplaceDeadCallsVisitor : public clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor> {
public:
  /// Visitor replaces all dead calls to previously removed functions
  ReplaceDeadCallsVisitor(clang::ASTContext *C, std::set<clang::QualType> *neededTypes);

  /// Initializes the traversal
  virtual bool VisitTranslationUnit(clang::TranslationUnitDecl *D);

  /// Default Visit function for all Declarations
  virtual bool VisitDecl(clang::Decl *D);

  /// Primary Visit needed to replace necessary calls with matching verifers
  virtual bool VisitCallExpr(clang::CallExpr *E);

  /// Instructs the visitor wether to recurse depth or breadth first
  bool shouldTraversePostOrder();

private:
  clang::ASTContext *_C;
  std::set<clang::QualType> *_NeededTypes;
};
