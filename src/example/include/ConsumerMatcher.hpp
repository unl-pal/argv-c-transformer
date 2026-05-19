#pragma once

#include <clang/AST/ASTConsumer.h>

// TODO - LEARN
// What is the better use case for AST Matcher VS Visitor?
class ConsumerMatcher : public clang::ASTConsumer {
public:
  void HandleTranslationUnit(clang::ASTContext &Context) override;
};
