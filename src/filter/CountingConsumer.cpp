#include "CountingVisitor.hpp"
#include "CountingConsumer.hpp"

#include <llvm/Support/raw_ostream.h>

CountingConsumer::CountingConsumer(const std::vector<unsigned int> &types,
    std::unordered_map<std::string, CountNodesVisitor::attributes *> *toFilter)
    : _Types(types), _ToFilter(toFilter) {}

void CountingConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  CountNodesVisitor Visitor(&Context, _Types, _ToFilter);
  // Context.getTranslationUnitDecl()->dumpColor();
  Visitor.TraverseTranslationUnitDecl(Context.getTranslationUnitDecl());
  // Visitor.PrintReport("unknown");
  // for (auto &entry : *_ToFilter) {
  //   llvm::outs() << entry.first << "\n";
  // }
}
