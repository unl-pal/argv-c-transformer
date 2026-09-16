// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConfigParser.hpp"
#include "HeaderClosure.hpp"
#include "SplitConsumer.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <map>
#include <memory>
#include <string>
#include <utility>

/**
 * @brief ASTFrontendAction that runs the full filter consumer chain.
 *
 * Wires together the consumer chain (count → filter → split-and-emit) over a
 * single parsed AST. Unlike the other pipeline stages, this action owns no
 * shared Rewriter: SplitConsumer builds one fresh per surviving function and
 * writes each result through {@code writeOutput} as soon as it's rendered.
 */
class FilterAction : public clang::ASTFrontendAction {
public:
  /**
   * @brief Constructs the action with the shared pipeline state.
   *
   * @param complexityConfig  Per-metric [min, max] ranges owned by {@code Filterer}.
   * @param featureConfig     Per-feature require/forbid/ignore gates owned by
   *                          {@code Filterer}.
   * @param writeOutput       Called once per surviving function with its name
   *                          and final source text; {@code Filterer} writes
   *                          each to its own file.
   */
  FilterAction(std::map<std::string, std::pair<int, int>> *complexityConfig,
               std::map<std::string, FeatureGate> *featureConfig, SplitOutputWriter writeOutput);

  /**
   * @brief Builds a {@code MultiplexConsumer} containing the count → filter →
   * split chain.
   *
   * Creates the shared state ({@code toFilter}, {@code toRemove}) as
   * {@code shared_ptr}s and hands them to the consumers in pipeline order;
   * each is freed once the last owning consumer is destroyed.
   *
   * @param compiler  The active compiler instance.
   * @param filename  Path of the file being processed.
   * @return Owning pointer to the multiplexed consumer.
   */
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &compiler,
                                                        llvm::StringRef filename) override;

  /**
   * @brief Registers the header-closure preprocessor callback.
   *
   * Must run before any directive or macro is lexed, so it's wired here
   * rather than in {@code CreateASTConsumer}.
   *
   * @param compiler  The active compiler instance.
   * @return Result of the parent implementation.
   */
  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

private:
  std::map<std::string, std::pair<int, int>> *_ComplexityConfig;
  std::map<std::string, FeatureGate> *_FeatureConfig;
  SplitOutputWriter _WriteOutput;
  /// Shared between LocalHeaderPP (fills it during preprocessing) and
  /// SplitConsumer (reads it once the AST is complete, once per target).
  std::shared_ptr<HeaderClosureState> _ClosureState;
};

/**
 * @brief Carries pipeline state into Clang's tool runner.
 *
 * {@code ClangTool::run()} only calls {@code create()} on a
 * {@code FrontendActionFactory}, so this subclass stores the config maps and
 * output writer needed to construct each {@code FilterAction}.
 */
class FrontendFactoryWithArgs : public clang::tooling::FrontendActionFactory {
public:
  /**
   * @brief Constructs the factory, binding the shared pipeline state.
   *
   * @param complexityConfig  Pointer to the per-metric [min, max] map owned by {@code Filterer}.
   * @param featureConfig     Pointer to the per-feature gate map owned by {@code Filterer}.
   * @param writeOutput       Sink for each surviving function's split output.
   */
  FrontendFactoryWithArgs(std::map<std::string, std::pair<int, int>> *complexityConfig,
                          std::map<std::string, FeatureGate> *featureConfig,
                          SplitOutputWriter writeOutput);

  /**
   * @brief Called by {@code ClangTool} once per source file to create the action.
   *
   * Returns a new {@code FilterAction} loaded with the config and output writer.
   *
   * @return Owning pointer to the created action.
   */
  std::unique_ptr<clang::FrontendAction> create() override;

private:
  std::map<std::string, std::pair<int, int>> *_ComplexityConfig;
  std::map<std::string, FeatureGate> *_FeatureConfig;
  SplitOutputWriter _WriteOutput;
};
