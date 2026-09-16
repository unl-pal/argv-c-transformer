// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "CountingConsumer.hpp"
#include "CountingVisitor.hpp"
#include "FilterAction.hpp"
#include "FilterFunctionsConsumer.hpp"
#include "HeaderClosure.hpp"
#include "SplitConsumer.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

FilterAction::FilterAction(std::map<std::string, std::pair<int, int>> *complexityConfig,
                           std::map<std::string, FeatureGate> *featureConfig,
                           SplitOutputWriter writeOutput)
    : _ComplexityConfig(complexityConfig), _FeatureConfig(featureConfig),
      _WriteOutput(std::move(writeOutput)), _ClosureState(std::make_shared<HeaderClosureState>()) {
}

std::unique_ptr<clang::ASTConsumer>
FilterAction::CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef /*filename*/) {
  compiler.createASTContext();

  auto toFilter = std::make_shared<std::unordered_map<std::string, CountingVisitor::attributes>>();
  auto toRemove = std::make_shared<std::vector<std::string>>();

  // unique_ptr can't be copied, so the vector must be moved into MultiplexConsumer.
  // Building a named local makes that std::move explicit and unambiguous.
  std::vector<std::unique_ptr<clang::ASTConsumer>> consumers;
  consumers.emplace_back(std::make_unique<CountingConsumer>(toFilter));
  consumers.emplace_back(std::make_unique<FilterFunctionsConsumer>(
      toFilter, toRemove, _ComplexityConfig, _FeatureConfig));
  // Last: needs the final reject list FilterFunctionsConsumer produces.
  consumers.emplace_back(std::make_unique<SplitConsumer>(toRemove, _ClosureState, _WriteOutput));

  return std::make_unique<clang::MultiplexConsumer>(std::move(consumers));
}

bool FilterAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  // Registered here rather than in CreateASTConsumer so no directive or macro
  // is lexed before the callback is listening.
  clang::Preprocessor &pp = compiler.getPreprocessor();
  pp.addPPCallbacks(
      std::make_unique<LocalHeaderPP>(compiler.getSourceManager(), pp.getLangOpts(), _ClosureState));
  return clang::ASTFrontendAction::BeginSourceFileAction(compiler);
}

FrontendFactoryWithArgs::FrontendFactoryWithArgs(
    std::map<std::string, std::pair<int, int>> *complexityConfig,
    std::map<std::string, FeatureGate> *featureConfig, SplitOutputWriter writeOutput)
    : _ComplexityConfig(complexityConfig), _FeatureConfig(featureConfig),
      _WriteOutput(std::move(writeOutput)) {}

std::unique_ptr<clang::FrontendAction> FrontendFactoryWithArgs::create() {
  return std::make_unique<FilterAction>(_ComplexityConfig, _FeatureConfig, _WriteOutput);
}
