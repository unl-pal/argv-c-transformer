#include "AddVerifiersConsumer.hpp"
#include "AddVerifiersVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

AddVerifiersConsumer::AddVerifiersConsumer(llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes) : _Output(output), _NeededTypes(neededTypes) {}

void AddVerifiersConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  AddVerifiersVisitor Visitor(&Context, _Output, _NeededTypes);
  Visitor.HandleTranslationUnit(Context.getTranslationUnitDecl());
}
