// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "CountingVisitor.hpp"
#include "FilterFunctionsConsumer.hpp"
#include "VerifierNames.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <stdexcept>
#include <unordered_map>

namespace {

// Looks up a named field on the complexity axis. Throws if `name` isn't one
// of the known metrics — a config key typo should surface loudly, not
// silently no-op.
int complexityField(const CountingVisitor::ComplexityCounts &c, const std::string &name) {
  if (name == "CallFunc")
    return c.CallFunc;
  if (name == "ForLoops")
    return c.ForLoops;
  if (name == "IfStmt")
    return c.IfStmt;
  if (name == "Param")
    return c.Param;
  if (name == "WhileLoops")
    return c.WhileLoops;
  throw std::invalid_argument("unknown complexity metric: " + name);
}

// Looks up a named flag on the feature axis. Throws if `name` isn't one of
// the known features, for the same reason as complexityField above.
bool featureField(const CountingVisitor::FeatureFlags &f, const std::string &name) {
  if (name == "Concurrency")
    return f.Concurrency;
  if (name == "FloatingPoint")
    return f.FloatingPoint;
  throw std::invalid_argument("unknown feature: " + name);
}

} // namespace

FilterFunctionsConsumer::FilterFunctionsConsumer(
    std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> toFilter,
    std::shared_ptr<std::vector<std::string>> toRemove,
    std::map<std::string, std::pair<int, int>> *complexityConfig,
    std::map<std::string, FeatureGate> *featureConfig)
    : _ToFilter(toFilter), _ToRemove(toRemove), _ComplexityConfig(complexityConfig),
      _FeatureConfig(featureConfig) {}

void FilterFunctionsConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  FilterFunctions(context);
}

void FilterFunctionsConsumer::FilterFunctions(clang::ASTContext &context) {
  if (_ToFilter->empty())
    return;

  // Build name → FunctionDecl* so the param-type check below can look up
  // the actual declaration for each function in _ToFilter.
  clang::SourceManager &mgr = context.getSourceManager();
  std::unordered_map<std::string, const clang::FunctionDecl *> declByName;
  for (clang::Decl *decl : context.getTranslationUnitDecl()->decls()) {
    const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (func && func->isThisDeclarationADefinition() && mgr.isInMainFile(func->getLocation()))
      declByName[func->getNameAsString()] = func;
  }

  for (const std::pair<const std::string, CountingVisitor::attributes> &func : *_ToFilter) {
    std::string key = func.first;
    const CountingVisitor::attributes &attr = func.second;
    if (key == "Program")
      continue;

    bool reject = false;
    for (const auto &[name, range] : *_ComplexityConfig) {
      int value = complexityField(attr.Complexity, name);
      if (value < range.first || value > range.second) {
        reject = true;
        break;
      }
    }
    if (!reject) {
      for (const auto &[name, gate] : *_FeatureConfig) {
        bool present = featureField(attr.Features, name);
        if ((gate == FeatureGate::Require && !present) ||
            (gate == FeatureGate::Forbid && present)) {
          reject = true;
          break;
        }
      }
    }
    if (reject) {
      _ToRemove->push_back(key);
      continue;
    }

    // All threshold checks passed — now check whether every parameter has a
    // nondet equivalent. If any param type is unsupported (pointer, struct,
    // etc.), strip the body so HavocCallsVisitor can still use the return
    // type from the remaining declaration. main is exempt here: its argc/argv
    // params are handled specially by MainGenConsumer.
    if (key != "main" && declByName.contains(key)) {
      for (auto parm : declByName.at(key)->parameters()) {
        if (!verifierSuffixForType(parm->getOriginalType())) {
          _ToRemove->push_back(key);
          break;
        }
      }
    }
  }
}
