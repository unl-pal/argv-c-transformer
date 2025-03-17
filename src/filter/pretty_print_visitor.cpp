#include "include/pretty_print_visitor.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>

PrintASTVisitor::PrintASTVisitor(clang::ASTContext *Context) : _Context(Context) {}

bool PrintASTVisitor::VisitNamedDecl(clang::NamedDecl *D) {
  std::string name = D->getNameAsString();
  llvm::outs() << "VisitNamedDecl";
  ++recursionDepth;
  // Indent based on recursion depth
  for (int i = 0; i <  recursionDepth; ++i) {
    llvm::outs() << "  ";
  }
  llvm::outs() << name << "\n";
  bool result = clang::RecursiveASTVisitor<PrintASTVisitor>::VisitNamedDecl(D);
  --recursionDepth;
  return result;
}

bool PrintASTVisitor::TraverseDecl(clang::Decl *D) {
  /*if (D->isInStdNamespace()) */
  if (_Context->getSourceManager().isInMainFile(D->getLocation()) == true) {
    /*clang::Decl::Kind::Import*/
    llvm::outs() << "Did it!! ================================" << "\n";
    return false;
  }
  llvm::outs() << "TraverseDecl";
  ++recursionDepth;
  for (int i = 0; i <  recursionDepth; ++i) {
    llvm::outs() << "  ";
  }
  llvm::outs() << D->getDeclKindName();
  llvm::outs() << "\n";
  bool result = clang::RecursiveASTVisitor<PrintASTVisitor>::TraverseDecl(D);
  --recursionDepth;
  return result;
}

bool PrintASTVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *Declaration) {
  clang::FullSourceLoc FullLocation = _Context->getFullLoc(Declaration->getBeginLoc());
  _Context->PrintStats();
  Declaration->printName(llvm::outs());
  if (FullLocation.isValid()) {
    llvm::outs() << "Found node at "
      << FullLocation.getSpellingLineNumber() << ":"
      << FullLocation.getSpellingColumnNumber() << "\n";
  }
  return true;
}
