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
 * {@code extern <type> __VERIFIER_nondet_<suffix>(void);} declaration. Also emits
 * the {@code __VERIFIER_nondet_memory} extern and {@code __HAVOC_BLOCK_MAX}
 * macro when the "__havoc_memory" marker is present (stack-block pointer
 * havocking), the {@code __HAVOC_ARGC_*}/{@code __HAVOC_STR_MAX} bound
 * macros when {@code MainGenConsumer} recorded the "__havoc_argv" marker, and a
 * {@code reach_error()} definition (plus its own {@code #include <assert.h>})
 * when {@code AssertRewriter} recorded the "__reach_error" marker.
 */
class AddVerifiersConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param neededSuffixes Verifier suffixes collected by earlier consumers
   *        ({@code HavocCallsConsumer}, {@code MainGenConsumer}, {@code AssertRewriter}).
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
