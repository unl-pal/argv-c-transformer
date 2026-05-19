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
// Constructor for consumer that requires the parts needed for the visitor
// @param types - vector of the types the user wants ie bool, int, float, etc.
// @param toFilter - map of functions to attributes that is used for the report
  CountingConsumer(const std::vector<unsigned int> &types,
   std::unordered_map<std::string, CountNodesVisitor::attributes *> *toFilter);

// Creates and runs the Counting Visitor to analyze all 
// functions in the code and report the number of _ in each function
// @param context - astContext needed for the tree traversal
  void HandleTranslationUnit(clang::ASTContext &Context);

private:
  const std::vector<unsigned int> &_Types;
  std::unordered_map<std::string, CountNodesVisitor::attributes *> *_ToFilter;
};
