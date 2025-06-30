#include "ReplaceDeadCallsConsumer.hpp"
#include "ReplaceDeadCallsVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <llvm/Support/raw_ostream.h>

ReplaceDeadCallsConsumer::ReplaceDeadCallsConsumer(std::set<clang::QualType> *neededTypes) : _NeededTypes(neededTypes) {
}

void ReplaceDeadCallsConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  llvm::outs() << "Running the ReplaceDeadCalls\n";
  ReplaceDeadCallsVisitor Visitor(&Context, _NeededTypes);
  Visitor.VisitTranslationUnit(Context.getTranslationUnitDecl());
  llvm::outs() << "Ran the ReplaceDeadCalls\n";
}
