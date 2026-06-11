#include "HavocCallsConsumer.hpp"
#include "HavocCallsVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>

HavocCallsConsumer::HavocCallsConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                                       clang::Rewriter &rewriter)
    : _NeededSuffixes(neededSuffixes), _Rewriter(rewriter) {}

void HavocCallsConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  HavocCallsVisitor Visitor(&Context, _NeededSuffixes, _Rewriter);
  Visitor.VisitTranslationUnit(Context.getTranslationUnitDecl());
}
