#include "CountingVisitor.hpp"
#include "CountingConsumer.hpp"

#include <llvm/Support/raw_ostream.h>

CountingConsumer::CountingConsumer(const std::vector<unsigned int> &types,
    std::unordered_map<std::string, CountNodesVisitor::attributes *> *toFilter)
    : _Types(types), _ToFilter(toFilter) {}

void CountingConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  CountNodesVisitor Visitor(&Context, _Types, _ToFilter);
  Visitor.TraverseTranslationUnitDecl(Context.getTranslationUnitDecl());
}
