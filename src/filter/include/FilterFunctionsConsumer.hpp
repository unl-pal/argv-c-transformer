#pragma once

#include <CountingVisitor.hpp>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <string>
#include <unordered_map>
#include <vector>
class FilterFunctionsConsumer : public clang::ASTConsumer {
public:
  FilterFunctionsConsumer(
    std::unordered_map<std::string, CountNodesVisitor::attributes *> *toFilter,
    std::vector<std::string> *toRemove, std::map<std::string, int> *config);

  void HandleTranslationUnit(clang::ASTContext &context);

	void FilterFunctions();

private:
std::unordered_map<std::string, CountNodesVisitor::attributes*> *_ToFilter;
std::vector<std::string> *_ToRemove;
std::map<std::string, int> *_Config;
};
