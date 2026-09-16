// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "HeaderClosure.hpp"

#include <clang/AST/ASTConsumer.h>
#include <functional>
#include <llvm/ADT/StringRef.h>
#include <memory>
#include <string>
#include <vector>

/** @brief Sink for one split output: the surviving function's name and its
 *  rendered source text. */
using SplitOutputWriter = std::function<void(const std::string &, llvm::StringRef)>;

/**
 * @brief Emits one filtered output per surviving function, so each is
 * harnessed and verified independently (issue #90).
 *
 * Runs last in the filter chain, after {@code FilterFunctionsConsumer} has
 * finalized {@code toRemove}. For every function definition in the main file
 * that isn't already rejected, builds a fresh Rewriter treating every OTHER
 * surviving function exactly as if the filter had rejected it too (body
 * stripped to {@code ;} via RemoveVisitor) — indistinguishable, to every later
 * stage, from a function that failed the thresholds outright. The header
 * closure then runs over that target-specific view, so each split file only
 * carries what its one live function actually needs.
 */
class SplitConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer.
   *
   * @param toRemove     Functions the filter rejected, from FilterFunctionsConsumer.
   * @param closureState Preprocessor-collected state from LocalHeaderPP, shared
   *                     read-only across every target's closure pass.
   * @param writeOutput  Called once per surviving function with its name and
   *                     final source text.
   */
  SplitConsumer(std::shared_ptr<std::vector<std::string>> toRemove,
                std::shared_ptr<HeaderClosureState> closureState, SplitOutputWriter writeOutput);

  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  std::shared_ptr<std::vector<std::string>> _ToRemove;
  std::shared_ptr<HeaderClosureState> _ClosureState;
  SplitOutputWriter _WriteOutput;
};
