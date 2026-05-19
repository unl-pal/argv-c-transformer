#include "AddVerifiersConsumerFilter.hpp"
#include "AddVerifiersVisitorFilter.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>

AddVerifiersConsumerFilter::AddVerifiersConsumerFilter(
  llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes,
  clang::Rewriter &rewriter)
    : _Output(output), _NeededTypes(neededTypes), _Rewriter(rewriter) {}

void AddVerifiersConsumerFilter::HandleTranslationUnit(clang::ASTContext &Context) {
  llvm::outs() << "Running the Verifiers\n";
  if (_NeededTypes->size()) {
    AddVerifiersVisitorFilter Visitor(&Context, _Output, _NeededTypes, _Rewriter);
    Visitor.HandleTranslationUnit(Context.getTranslationUnitDecl());
    llvm::outs() << "Ran the Verifiers\n";
  }
}
