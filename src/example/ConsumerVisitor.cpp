#include "include/ConsumerVisitor.hpp"

void ConsumerVisitor::HandleTranslationUnit(clang::ASTContext &Context) {
  Visitor.HandleTranslationUnit(Context.getTranslationUnitDecl());
}
