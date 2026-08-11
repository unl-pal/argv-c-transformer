// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConfigParser.hpp"
#include "CountingVisitor.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @brief ASTFrontendAction for the verify stage's re-check of a transformed
 * file.
 *
 * Reparses the generated source from scratch and runs: CountingConsumer
 * (fresh counts), VerifyFunctionsConsumer (re-apply thresholds),
 * RemoveConsumer (strip rejected bodies), HarnessRepairConsumer (drop
 * removed functions' calls from main). The repaired buffer is written to
 * {@code output}.
 *
 * The counts map and removal list are owned by the Verifier driver so it can
 * read them back after the tool run (e.g. for property selection); the
 * empty-harness discard is decided separately, from the written output file.
 */
class VerifyAction : public clang::ASTFrontendAction {
public:
  VerifyAction(std::map<std::string, std::pair<int, int>> *complexityConfig,
               std::map<std::string, FeatureGate> *featureConfig,
               std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> counts,
               std::shared_ptr<std::vector<std::string>> toRemove, llvm::raw_fd_ostream &output);

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &compiler,
                                                        llvm::StringRef filename) override;

  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

  /** Flushes the repaired buffer to the output file. */
  void EndSourceFileAction() override;

private:
  std::map<std::string, std::pair<int, int>> *_ComplexityConfig;
  std::map<std::string, FeatureGate> *_FeatureConfig;
  std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> _Counts;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
  clang::Rewriter _Rewriter;
  llvm::raw_fd_ostream &_Output;
};

/**
 * @brief Carries the verify stage's shared state into Clang's tool runner,
 * mirroring the filter's FrontendFactoryWithArgs.
 */
class VerifyActionFactory : public clang::tooling::FrontendActionFactory {
public:
  VerifyActionFactory(
      std::map<std::string, std::pair<int, int>> *complexityConfig,
      std::map<std::string, FeatureGate> *featureConfig,
      std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> counts,
      std::shared_ptr<std::vector<std::string>> toRemove, llvm::raw_fd_ostream &output);

  std::unique_ptr<clang::FrontendAction> create() override;

private:
  std::map<std::string, std::pair<int, int>> *_ComplexityConfig;
  std::map<std::string, FeatureGate> *_FeatureConfig;
  std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> _Counts;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
  llvm::raw_fd_ostream &_Output;
};
