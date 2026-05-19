#pragma once

#include <clang/AST/ASTConsumer.h>
#include <string>

// TODO - LEARN
// What is the better use case for AST Matcher VS Visitor?
class RemoveNodeConsumer : public clang::ASTConsumer {
public:
  void HandleTranslationUnit(clang::ASTContext &Context);

private:
  std::string _Name;
};
