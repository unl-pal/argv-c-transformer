// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "CountingConsumer.hpp"
#include "CountingVisitor.hpp"
#include "FilterAction.hpp"
#include "FilterFunctionsConsumer.hpp"
#include "RemoveConsumer.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/TemplateName.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

FilterAction::FilterAction(std::map<std::string, std::pair<int, int>> *complexityConfig,
                           std::map<std::string, FeatureGate> *featureConfig,
                           llvm::raw_fd_ostream &output)
    : _ComplexityConfig(complexityConfig), _FeatureConfig(featureConfig), _Rewriter(),
      _Output(output) {}

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
  consumers.emplace_back(std::make_unique<RemoveConsumer>(_Rewriter, toRemove));

  return std::make_unique<clang::MultiplexConsumer>(std::move(consumers));
}

bool FilterAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  return clang::ASTFrontendAction::BeginSourceFileAction(compiler);
}

void FilterAction::EndSourceFileAction() {
  _Rewriter.getEditBuffer(getCompilerInstance().getSourceManager().getMainFileID()).write(_Output);
}

FrontendFactoryWithArgs::FrontendFactoryWithArgs(
    std::map<std::string, std::pair<int, int>> *complexityConfig,
    std::map<std::string, FeatureGate> *featureConfig, llvm::raw_fd_ostream &output)
    : _ComplexityConfig(complexityConfig), _FeatureConfig(featureConfig), _Output(output) {}

std::unique_ptr<clang::FrontendAction> FrontendFactoryWithArgs::create() {
  return std::make_unique<FilterAction>(_ComplexityConfig, _FeatureConfig, _Output);
}
