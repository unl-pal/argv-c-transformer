#pragma once

#include "Visitor.hpp"

#include <clang/AST/ASTConsumer.h>

// TODO - LEARN
// the consumer takes in compiler and filename, does this mean that it is
// handling the generation of the AST as well as starting the visitors on it?
class ConsumerVisitor : public clang::ASTConsumer {
public:
  void HandleTranslationUnit(clang::ASTContext &Context) override;

  // TODO - LEARN
  // The Visitor that the consumer is using has already been set in this case,
  // is that how it normally is? Can a consumer dispatch many visitors on the
  // tree or is that not kosher?
private:
  Visitor Visitor;
};

