// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/VerifyFunctionsConsumer.hpp"

#include "CountingVisitor.hpp"
#include "DebugLog.hpp"
#include "VerifierNames.hpp"

VerifyFunctionsConsumer::VerifyFunctionsConsumer(
    std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> counts,
    std::shared_ptr<std::vector<std::string>> toRemove,
    std::map<std::string, std::pair<int, int>> *complexityConfig,
    std::map<std::string, FeatureGate> *featureConfig)
    : _Counts(counts), _ToRemove(toRemove), _ComplexityConfig(complexityConfig),
      _FeatureConfig(featureConfig) {}

void VerifyFunctionsConsumer::HandleTranslationUnit(clang::ASTContext &) {
  for (const auto &[name, attr] : *_Counts) {
    // "Program" is file scope, main is the synthesized harness, and
    // __VERIFIER_*/__havoc_* are injected scaffolding — none are subject to
    // the re-check. Everything else (including original_main) is program
    // logic that must still meet the configured thresholds.
    if (name == "Program" || name == "main" || isVerifierGenerated(name))
      continue;

    bool reject = false;
    for (const auto &[metric, range] : *_ComplexityConfig) {
      int value = complexityField(attr.Complexity, metric);
      if (value < range.first || value > range.second) {
        debugLog(2, "[verify] " + name + ": " + metric + " = " + std::to_string(value) +
                        " outside [" + std::to_string(range.first) + "," +
                        std::to_string(range.second) + "] post-transform");
        reject = true;
        break;
      }
    }
    if (!reject) {
      for (const auto &[feature, gate] : *_FeatureConfig) {
        bool present = featureField(attr.Features, feature);
        if ((gate == FeatureGate::Require && !present) ||
            (gate == FeatureGate::Forbid && present)) {
          debugLog(2, "[verify] " + name + ": feature gate '" + feature +
                          "' violated post-transform");
          reject = true;
          break;
        }
      }
    }
    if (reject)
      _ToRemove->push_back(name);
  }
}
