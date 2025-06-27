#include "RemoveVisitor.hpp"
#include "RemoveConsumer.hpp"

#include <clang/ASTMatchers/ASTMatchFinder.h>

RemoveConsumer::RemoveConsumer(clang::Rewriter &rewriter, std::vector<std::string> *toRemove) : _Rewriter(rewriter), _toRemove(toRemove) {}

void RemoveConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  if (_toRemove->size()) {
    RemoveFuncVisitor Visitor(&Context, _Rewriter, *_toRemove);
    bool done = false;
    while (!done) {
      done = Visitor.TraverseTranslationUnitDecl(Context.getTranslationUnitDecl());
    }
  }
}
