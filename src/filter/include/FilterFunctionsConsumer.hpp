#pragma once

#include "CountingVisitor.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief ASTConsumer that decides which functions to remove based on counted attributes.
 *
 * Does not use a visitor — by the time this consumer runs, {@code CountingConsumer}
 * has already populated {@code toFilter} with per-function attribute counts. This
 * consumer just reads that map and applies the configured min/max thresholds, writing
 * any violating function names into {@code toRemove} for {@code RemoveConsumer} to act on.
 *
 * {@code "Program"} (file-scope counts) and {@code "main"} are always kept.
 */
class FilterFunctionsConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param toFilter  Map of function name → attribute counts, written by {@code CountingConsumer}.
   * @param toRemove  Output vector; names of functions that fail thresholds are appended.
   * @param config    Threshold map (min/max per attribute) owned by {@code Filterer}.
   */
  FilterFunctionsConsumer(std::unordered_map<std::string, CountingVisitor::attributes *> *toFilter,
                          std::vector<std::string> *toRemove, std::map<std::string, int> *config);

  /**
   * @brief Entry point called by Clang once the AST is fully parsed.
   *
   * Delegates entirely to {@code FilterFunctions()}.
   *
   * @param context  Unused; required by the {@code ASTConsumer} interface.
   */
  void HandleTranslationUnit(clang::ASTContext &context) override;

  /**
   * @brief Applies min/max thresholds to every function in {@code _ToFilter}.
   *
   * A function is added to {@code _ToRemove} on the first threshold violation
   * found. Checks max bounds before min bounds; order within each group does
   * not affect correctness since only one violation is needed for removal.
   */
  void FilterFunctions();

private:
  std::unordered_map<std::string, CountingVisitor::attributes *> *_ToFilter;
  std::vector<std::string> *_ToRemove;
  std::map<std::string, int> *_Config;
};
