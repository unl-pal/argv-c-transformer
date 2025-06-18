#include "AddVerifiersConsumer.hpp"
#include "CreateVerifiersVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

AddVerifiersConsumer::AddVerifiersConsumer(llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes) : _Output(output), _NeededTypes(neededTypes) {}

void AddVerifiersConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  CreateVerifiersVisitor Visitor(&Context, _Output, _NeededTypes);
  llvm::outs() << "Running the Handle TU in AddVerifiersConsumer\n";
    if (!Context.getTranslationUnitDecl()) {
    llvm::outs() << "Context is blank\n";
  }
  // Context.getTranslationUnitDecl()->dumpDeclContext();

  Visitor.HandleTranslationUnit(Context.getTranslationUnitDecl());
}
