#pragma once

#include <CountingVisitor.hpp>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <string>
#include <unordered_map>
#include <vector>

/// Consumer for filtering all counted functions and their attributes to find all the functions that should be removed
class FilterFunctionsConsumer : public clang::ASTConsumer {
public:
  /// @brief Constructor for consumer that sets the private variables used for
  /// filtering
  /// @detail this consumer does not use a Visitor or Handler, in the case of an
  /// ASTMatcher, instead doing the work itself. It is being done this way so as
  /// to operate on the same AST generated for the other filtering tasks
  /// @param toFilter - all functions that will be checked
  /// @param all functions that should be removed later
  FilterFunctionsConsumer(
    std::unordered_map<std::string, CountNodesVisitor::attributes *> *toFilter,
    std::vector<std::string> *toRemove, std::map<std::string, int> *config);

  /// Run all necessary tasks to filter
  /// this does not use a visitor or matcher but is called by the action when
  /// going through all consumer's HandleTranslationUnits, thus is used to
  /// trigger in the correct order of operations rather than at creation or
  /// needing a special call
  void HandleTranslationUnit(clang::ASTContext &context);

  /// Filter the Functions
  void FilterFunctions();

private:
std::unordered_map<std::string, CountNodesVisitor::attributes*> *_ToFilter;
std::vector<std::string> *_ToRemove;
std::map<std::string, int> *_Config;
};
