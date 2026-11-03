// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/VerifyAction.hpp"
#include "include/HarnessSplitConsumer.hpp"
#include "include/VerifyFunctionsConsumer.hpp"

#include "CountingConsumer.hpp"
#include "RemoveConsumer.hpp"

#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <memory>
#include <vector>

VerifyAction::VerifyAction(
    std::map<std::string, std::pair<int, int>> *complexityConfig,
    std::map<std::string, FeatureGate> *featureConfig,
    std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> counts,
    std::shared_ptr<std::vector<std::string>> toRemove, HarnessSplit &output)
    : _ComplexityConfig(complexityConfig), _FeatureConfig(featureConfig), _Counts(counts),
      _ToRemove(toRemove), _Rewriter(), _Output(output) {}

std::unique_ptr<clang::ASTConsumer>
VerifyAction::CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef /*filename*/) {
  compiler.createASTContext();

  std::vector<std::unique_ptr<clang::ASTConsumer>> consumers;
  consumers.emplace_back(std::make_unique<CountingConsumer>(_Counts));
  consumers.emplace_back(std::make_unique<VerifyFunctionsConsumer>(
      _Counts, _ToRemove, _ComplexityConfig, _FeatureConfig));
  consumers.emplace_back(std::make_unique<RemoveConsumer>(_Rewriter, _ToRemove));
  consumers.emplace_back(std::make_unique<HarnessSplitConsumer>(_Rewriter, _ToRemove, _Output));

  return std::make_unique<clang::MultiplexConsumer>(std::move(consumers));
}

bool VerifyAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  return clang::ASTFrontendAction::BeginSourceFileAction(compiler);
}

VerifyActionFactory::VerifyActionFactory(
    std::map<std::string, std::pair<int, int>> *complexityConfig,
    std::map<std::string, FeatureGate> *featureConfig,
    std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> counts,
    std::shared_ptr<std::vector<std::string>> toRemove, HarnessSplit &output)
    : _ComplexityConfig(complexityConfig), _FeatureConfig(featureConfig), _Counts(counts),
      _ToRemove(toRemove), _Output(output) {}

std::unique_ptr<clang::FrontendAction> VerifyActionFactory::create() {
  return std::make_unique<VerifyAction>(_ComplexityConfig, _FeatureConfig, _Counts, _ToRemove,
                                        _Output);
}
