#include "AddVerifiersConsumer.hpp"
#include "AddVerifiersVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>

AddVerifiersConsumer::AddVerifiersConsumer(
  llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes,
  clang::Rewriter &rewriter)
    : _Output(output), _NeededTypes(neededTypes), _Rewriter(rewriter) {}

void AddVerifiersConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  llvm::outs() << "Running the Verifiers\n";
  if (_NeededTypes->size()) {
    AddVerifiersVisitor Visitor(&Context, _Output, _NeededTypes, _Rewriter);
    Visitor.HandleTranslationUnit(Context.getTranslationUnitDecl());
    llvm::outs() << "Ran the Verifiers\n";
  }
}
