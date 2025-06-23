#include "CountingVisitor.hpp"
#include "CountingConsumer.hpp"

CountingConsumer::CountingConsumer(std::vector<unsigned int> types) : _Types(types) {}

void CountingConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  CountNodesVisitor Visitor(&Context, _Types);
  Visitor.TraverseTranslationUnitDecl(Context.getTranslationUnitDecl());
}
