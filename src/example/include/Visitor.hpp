#pragma once

#include <clang/AST/RecursiveASTVisitor.h>

class Visitor : public clang::RecursiveASTVisitor<Visitor> {
public:
  // Why all the virtual functions for this one? 
  // What is the reason they apparently make sense for this use case?
  // Override this, it is used by the consumer to start the traversal process
  virtual bool HandleTranslationUnit(clang::TranslationUnitDecl *D);

  // Basic Decl visit
  // Other types are dictated in td files supplied by the precompiler and are
  // not visible online but are there
  virtual bool VisitDecl(clang::Decl *D);

  // TODO - LEARN
  // Walk up handles the ____ of the logic.... yeah I need this one to answer a
  // few more questions for me before it is of use to us WalkUpFrom<Type>
  // Does either pre or post order depending on the should_() function below
  //  Pre - calls walkup then recursively visits all children
  //  Post - recursively visits all children then calls walkUp
  virtual bool WalkUpFromDecl(clang::Decl *D);

  // Traverse differes from visit - how I do not know
  // TODO - LEARN
  // Traverse<Type> exists for many and is used to do the ___ of the logic
  // this needs more research as well
  virtual bool TraverseDecl(clang::Decl *D);

  // Basic Visit to override for statements
  // What types are available in the td files for stmts and how do they compare
  //  to the decls available
  virtual bool VisitStmt(clang::Stmt *S);

  // TODO - LEARN
  // Dictates whether the visitor traverses in pre or post order
  // This has the potential to fix the issues on the deleter when it was
  // attempting to do a single run instead of many  as this would allow deletion
  // in reverse order instead
  bool shouldTraversePostOrder();
  // TODO - LEARN
  // What other 'Shoulds' exist and what behaviors can they dictate
  // Need to Re-Read the whole of the VisitDecl function and see what the parts
  // are and every function call made and variable referenced
};

