// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

/**
 * @brief ASTConsumer that inserts {@code extern __VERIFIER_nondet_*} declarations.
 *
 * Runs last in the transform consumer chain. For every verifier suffix recorded
 * by earlier consumers (havoc calls, main generation), inserts an
 * {@code extern <type> __VERIFIER_nondet_<suffix>(void);} declaration at the top
 * of the main file, skipping any that the filter step already injected. Also emits
 * helper definitions for {@code __havoc_block} and {@code __havoc_cstring} when
 * those markers are present in the suffix set.
 */
class AddVerifiersConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param neededSuffixes Verifier suffixes collected by earlier consumers
   *        ({@code HavocCallsConsumer}, {@code MainGenConsumer}).
   * @param rewriter       Shared rewriter for modifying the source buffer.
   */
  AddVerifiersConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                       clang::Rewriter &rewriter);

  /**
   * @brief Inserts the extern declarations and helper definitions.
   *
   * @param Context The AST context for the translation unit being transformed.
   */
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
};
