#pragma once

#include "CountingVisitor.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <vector>

class CountingConsumer : public clang::ASTConsumer {
public:
  CountingConsumer(const std::vector<unsigned int> &types,
   std::unordered_map<std::string, CountNodesVisitor::attributes *> *toFilter);

  void HandleTranslationUnit(clang::ASTContext &Context);

private:
  const std::vector<unsigned int> &_Types;
  std::unordered_map<std::string, CountNodesVisitor::attributes *> *_ToFilter;
};
