// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CountingVisitor.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <memory>
#include <unordered_map>

/**
 * @brief ASTConsumer that drives the counting pass over the AST.
 *
 * Receives the parsed AST from Clang, then delegates the actual traversal
 * to {@code CountingVisitor}. By the time {@code HandleTranslationUnit}
 * returns, {@code toFilter} is populated with per-function attribute counts
 * for the next consumer ({@code FilterFunctionsConsumer}) to evaluate.
 */
class CountingConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param toFilter  Output map; visitor writes function name → attribute counts.
   */
  explicit CountingConsumer(
      std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> toFilter);

  /**
   * @brief Entry point called by Clang once the AST is fully parsed.
   *
   * Constructs a {@code CountingVisitor} and runs it over the translation
   * unit root, populating {@code _ToFilter}.
   *
   * @param context  The AST context for this translation unit.
   */
  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> _ToFilter;
};
