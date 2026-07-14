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
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <string>
#include <utility>

/**
 * @brief ASTFrontendAction that runs the full filter consumer chain.
 *
 * Wires together the three consumers (count → filter → strip bodies) over a
 * single parsed AST. A shared {@code Rewriter} accumulates all edits; the
 * final buffer is flushed to the output file in {@code EndSourceFileAction}.
 */
class FilterAction : public clang::ASTFrontendAction {
public:
  /**
   * @brief Constructs the action with the shared pipeline state.
   *
   * @param complexityConfig  Per-metric [min, max] ranges owned by {@code Filterer}.
   * @param featureConfig     Per-feature require/forbid/ignore gates owned by
   *                          {@code Filterer}.
   * @param output            Destination file stream for the filtered output.
   */
  FilterAction(std::map<std::string, std::pair<int, int>> *complexityConfig,
               std::map<std::string, FeatureGate> *featureConfig, llvm::raw_fd_ostream &output);

  /**
   * @brief Builds a {@code MultiplexConsumer} containing all three filter passes.
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
   * @brief Attaches the {@code Rewriter} to the compiler before AST actions run.
   *
   * Must be called before any consumer can make edits, because the Rewriter
   * needs the {@code SourceManager} and {@code LangOptions} that only exist
   * once the compiler instance is fully set up.
   *
   * @param compiler  The active compiler instance.
   * @return Result of the parent implementation.
   */
  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

  /**
   * @brief Flushes the Rewriter's edited buffer to the output file.
   *
   * Called after all consumers have finished. Writes the modified source
   * text, with the bodies of filtered-out functions stripped, to the
   * destination stream.
   */
  void EndSourceFileAction() override;

private:
  std::map<std::string, std::pair<int, int>> *_ComplexityConfig;
  std::map<std::string, FeatureGate> *_FeatureConfig;
  clang::Rewriter _Rewriter;
  llvm::raw_fd_ostream &_Output;
};

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
