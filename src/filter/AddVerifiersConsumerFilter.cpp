#include "AddVerifiersConsumerFilter.hpp"
#include "AddVerifiersVisitorFilter.hpp"

AddVerifiersConsumerFilter::AddVerifiersConsumerFilter(
    llvm::raw_fd_ostream &output, std::shared_ptr<std::set<std::string>> neededTypes,
    clang::Rewriter &rewriter)
    : _Output(output), _NeededTypes(neededTypes), _Rewriter(rewriter) {}

void AddVerifiersConsumerFilter::HandleTranslationUnit(clang::ASTContext &context) {
  if (!_NeededTypes->empty()) {
    AddVerifiersVisitorFilter visitor(&context, _NeededTypes, _Rewriter);
    visitor.HandleTranslationUnit(context.getTranslationUnitDecl());
  }
}
