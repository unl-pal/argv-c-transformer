#include "ReplaceDeadCallsConsumer.hpp"
#include "ReplaceCallsVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <llvm/Support/raw_ostream.h>

ReplaceDeadCallsConsumer::ReplaceDeadCallsConsumer(std::set<clang::QualType> *neededTypes) : _NeededTypes(neededTypes) {
  llvm::outs() << "Created ReplaceDeadCallsConsumer\n";
}

void ReplaceDeadCallsConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  ReplaceDeadCallsVisitor Visitor(&Context, _NeededTypes);
  llvm::outs() << "Running the ReplaceDeadCallsConsumer\n";
  Visitor.VisitTranslationUnit(Context.getTranslationUnitDecl());
}
