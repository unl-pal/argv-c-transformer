#include "RemoveVisitor.hpp"
#include "RemoveConsumer.hpp"

#include <clang/AST/Type.h>

RemoveConsumer::RemoveConsumer(clang::Rewriter &rewriter,
                               std::shared_ptr<std::vector<std::string>> toRemove,
                               std::shared_ptr<std::set<std::string>> neededTypes)
    : _Rewriter(rewriter), _ToRemove(toRemove), _NeededTypes(neededTypes) {}

void RemoveConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  if (!_ToRemove->empty()) {
    RemoveVisitor visitor(&context, _Rewriter, _ToRemove, _NeededTypes);
    visitor.TraverseDecl(context.getTranslationUnitDecl());
  }
}
