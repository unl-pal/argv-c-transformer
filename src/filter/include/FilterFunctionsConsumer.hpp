// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConfigParser.hpp"
#include "CountingVisitor.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @brief ASTConsumer that decides which functions to remove based on counted attributes.
 *
 * Does not use a visitor - by the time this consumer runs, {@code CountingConsumer}
 * has already populated {@code toFilter} with per-function attribute counts. This
 * consumer just reads that map and applies the configured complexity ranges and
 * feature gates, writing any violating function names into {@code toRemove} for
 * {@code RemoveConsumer} to act on.
 */
class FilterFunctionsConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param toFilter          Map of function name → attribute counts, written by
   *                          {@code CountingConsumer}.
   * @param toRemove          Output vector; names of functions that fail thresholds
   *                          are appended.
   * @param complexityConfig  Per-metric [min, max] ranges, owned by {@code Filterer}.
   * @param featureConfig     Per-feature require/forbid/ignore gates, owned by
   *                          {@code Filterer}.
   */
  FilterFunctionsConsumer(
      std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> toFilter,
      std::shared_ptr<std::vector<std::string>> toRemove,
      std::map<std::string, std::pair<int, int>> *complexityConfig,
      std::map<std::string, FeatureGate> *featureConfig);

  void HandleTranslationUnit(clang::ASTContext &context) override;

  /**
   * @brief Applies complexity ranges, feature gates, and param-type checks to
   * every function.
   *
   * A function is added to {@code _ToRemove} on the first violation found:
   * first every complexity metric is checked against its configured [min, max]
   * range, then (if still in the running) every feature flag is checked
   * against its configured gate. After that, any function whose parameters
   * include a type with no {@code __VERIFIER_nondet_*} equivalent is also
   * removed (body stripped so the declaration survives for return-type
   * resolution in the transform step).
   */
  void FilterFunctions(clang::ASTContext &context);

private:
  std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> _ToFilter;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
  std::map<std::string, std::pair<int, int>> *_ComplexityConfig;
  std::map<std::string, FeatureGate> *_FeatureConfig;
};
