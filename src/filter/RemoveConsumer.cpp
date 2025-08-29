#include "RemoveVisitor.hpp"
#include "RemoveConsumer.hpp"

#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/Support/raw_ostream.h>
#include <vector>

RemoveConsumer::RemoveConsumer(clang::Rewriter          &rewriter,
                               std::vector<std::string> *toRemove,
                               std::set<clang::QualType> *neededTypes)
    : _Rewriter(rewriter), _toRemove(toRemove), _NeededTypes(neededTypes) {}

void RemoveConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  llvm::outs() << "Starting Removal\n";
  if (_toRemove->size()) {
    RemoveFuncVisitor Visitor(&Context, _Rewriter, _toRemove, _NeededTypes);
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
  llvm::outs() << "Ending Removal\n";
}
