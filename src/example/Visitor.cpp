#include "Visitor.hpp"

bool Visitor::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  D->dump();
  return true;
}

bool Visitor::VisitDecl(clang::Decl *D) {
  D->dump();
  return true;
}

bool Visitor::WalkUpFromDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<Visitor>::WalkUpFromDecl(D);
}

bool Visitor::TraverseDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<Visitor>::TraverseDecl(D);
}

bool Visitor::VisitStmt(clang::Stmt *S) {
  S->dump();
  return true;
}

bool Visitor::shouldTraversePostOrder() {
  return true;
}
