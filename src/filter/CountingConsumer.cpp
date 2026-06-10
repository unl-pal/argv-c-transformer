#include "CountingConsumer.hpp"

#include <llvm/Support/raw_ostream.h>

CountingConsumer::CountingConsumer(
    const std::vector<unsigned int> &types,
    std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> toFilter)
    : _Types(types), _ToFilter(toFilter) {}

void CountingConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  CountingVisitor Visitor(&Context, _Types, _ToFilter);
  Visitor.TraverseTranslationUnitDecl(Context.getTranslationUnitDecl());
}
