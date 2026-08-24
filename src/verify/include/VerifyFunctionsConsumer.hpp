// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConfigParser.hpp"
#include "CountingVisitor.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @brief Re-applies the filter's per-function thresholds to a transformed
 * file's fresh counts.
 *
 * The transform can shrink a function below the thresholds it originally
 * passed (dropped void calls, pruned empty control flow), so the verify
 * stage re-checks the same config against counts taken from the generated
 * source. Differences from the filter's FilterFunctionsConsumer:
 *
 * - Generated artifacts are exempt: {@code main} (the synthesized harness)
 *   and {@code __VERIFIER_*} / {@code __havoc_*} definitions are pipeline
 *   scaffolding, not program logic, and must never be stripped.
 * - No parameter-supportability check: params were vetted by the filter and
 *   MainGenConsumer, and {@code original_main}'s argv is handled specially.
 *
 * Rejected names go into {@code toRemove} for RemoveConsumer (body → ;) and
 * HarnessRepairConsumer (drop the call from the generated main).
 */
class VerifyFunctionsConsumer : public clang::ASTConsumer {
public:
  VerifyFunctionsConsumer(
      std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> counts,
      std::shared_ptr<std::vector<std::string>> toRemove,
      std::map<std::string, std::pair<int, int>> *complexityConfig,
      std::map<std::string, FeatureGate> *featureConfig);

  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> _Counts;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
  std::map<std::string, std::pair<int, int>> *_ComplexityConfig;
  std::map<std::string, FeatureGate> *_FeatureConfig;
};
