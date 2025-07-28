#include "ReplaceDeadCallsConsumer.hpp"
#include "ReplaceDeadCallsVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Type.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>

ReplaceDeadCallsConsumer::ReplaceDeadCallsConsumer(std::set<clang::QualType> *neededTypes, clang::Rewriter &rewriter) : _NeededTypes(neededTypes), _Rewriter(rewriter) {
}

void ReplaceDeadCallsConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  llvm::outs() << "Running the ReplaceDeadCalls\n";
  ReplaceDeadCallsVisitor Visitor(&Context, _NeededTypes, _Rewriter);
  Visitor.VisitTranslationUnit(Context.getTranslationUnitDecl());
  llvm::outs() << "Ran the ReplaceDeadCalls\n";
}
