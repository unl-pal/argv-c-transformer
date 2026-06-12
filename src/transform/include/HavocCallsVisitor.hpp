#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

class HavocCallsVisitor : public clang::RecursiveASTVisitor<HavocCallsVisitor> {
public:
  /// Visitor that havocs every function call, making each function body
  /// intraprocedural: calls yielding a primitive become
  /// __VERIFIER_nondet_<type>(), pointer-returning calls become
  /// __VERIFIER_nondet_pointer(), and void calls are dropped.
  HavocCallsVisitor(clang::ASTContext *C, std::shared_ptr<std::set<std::string>> neededSuffixes,
                    clang::Rewriter &rewriter);

  /// Initializes the traversal
  virtual bool VisitTranslationUnit(clang::TranslationUnitDecl *D);

  /// Default Visit function for all Declarations
  virtual bool VisitDecl(clang::Decl *D);

  /// Primary Visit needed to replace calls with matching verifier nondets
  virtual bool VisitCallExpr(clang::CallExpr *E);

  /// Instructs the visitor whether to recurse depth or breadth first
  bool shouldTraversePostOrder();

private:
  clang::ASTContext *_C;
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
};
