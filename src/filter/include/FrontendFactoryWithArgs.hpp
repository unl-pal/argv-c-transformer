// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConfigParser.hpp"
#include "CountingVisitor.hpp"

#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/PCHContainerOperations.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <string>
#include <utility>

/**
 * @brief Carries pipeline state into Clang's tool runner.
 *
 * Clang's {@code ClangTool::run()} only knows how to call {@code create()} on
 * a {@code FrontendActionFactory}. This subclass stores the config maps and
 * output stream so that each {@code FilterAction} it creates has everything
 * it needs without those objects being globals.
 */
class FrontendFactoryWithArgs : public clang::tooling::FrontendActionFactory {
public:
  /**
   * @brief Constructs the factory, binding the shared pipeline state.
   *
   * @param complexityConfig  Pointer to the per-metric [min, max] map owned by {@code Filterer}.
   * @param featureConfig     Pointer to the per-feature gate map owned by {@code Filterer}.
   * @param output            Reference to the output stream for the filtered file.
   */
  FrontendFactoryWithArgs(std::map<std::string, std::pair<int, int>> *complexityConfig,
                          std::map<std::string, FeatureGate> *featureConfig,
                          llvm::raw_fd_ostream &output);

  /**
   * @brief Called by {@code ClangTool} once per source file to create the action.
   *
   * Returns a new {@code FilterAction} loaded with the config and output stream.
   *
   * @return Owning pointer to the created action.
   */
  std::unique_ptr<clang::FrontendAction> create() override;

private:
  std::map<std::string, std::pair<int, int>> *_ComplexityConfig;
  std::map<std::string, FeatureGate> *_FeatureConfig;
  llvm::raw_fd_ostream &_Output;
};
