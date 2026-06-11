#include "ReplaceDeadCallsConsumer.hpp"
#include "ReplaceDeadCallsVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>

/*
 * Dead calls are different from just the calls to removed functions and handle
 * C language specific features, static, inline and extern functions as well
 * clean up for the transformer generated or modified code
 */
ReplaceDeadCallsConsumer::ReplaceDeadCallsConsumer(
    std::shared_ptr<std::set<std::string>> neededSuffixes, clang::Rewriter &rewriter)
    : _NeededSuffixes(neededSuffixes), _Rewriter(rewriter) {}

void ReplaceDeadCallsConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  ReplaceDeadCallsVisitor Visitor(&Context, _NeededSuffixes, _Rewriter);
  Visitor.VisitTranslationUnit(Context.getTranslationUnitDecl());
}
