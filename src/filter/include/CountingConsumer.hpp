#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <vector>

class CountingConsumer : public clang::ASTConsumer {
public:
  CountingConsumer(std::vector<unsigned int> types);

  void HandleTranslationUnit(clang::ASTContext &Context);

private:
  std::vector<unsigned int> _Types;
};
