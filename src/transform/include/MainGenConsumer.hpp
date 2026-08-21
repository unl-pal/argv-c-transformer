// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "HavocBounds.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

/**
 * @brief ASTConsumer that generates the benchmark entry point.
 *
 * Inserts the {@code __HAVOC_*} bound macros and the
 * {@code #include "argv_c_harness.h"} prelude every transformed file needs
 *
 * Any pre-existing {@code main} (including its forward declarations) is renamed
 * to {@code original_main}, then a fresh {@code int main(void)} is appended that
 * calls every function defined in the file with {@code __VERIFIER_nondet_*}
 * arguments. Functions with a parameter type that has no nondet equivalent
 * (pointers, structs, ...), variadic functions, and functions whose body
 * {@code HavocCallsConsumer} found to have collapsed entirely to no-ops are
 * skipped. For {@code original_main(int, char**)}, a synthesized
 * {@code argc}/{@code argv} harness is generated instead of skipping.
 */
class MainGenConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param noOpFunctions Names of functions {@code HavocCallsConsumer} found
   *        to have an entirely no-op body; these are not harnessed.
   * @param rewriter      Shared rewriter for modifying the source buffer.
   * @param havoc         Bounds emitted as the __HAVOC_* macro definitions
   *                      ahead of the argv_c_harness.h #include.
   */
  MainGenConsumer(std::shared_ptr<std::set<std::string>> noOpFunctions, clang::Rewriter &rewriter,
                  const HavocBounds &havoc = {});

  /**
   * @brief Renames an existing {@code main} and appends the generated harness main.
   *
   * Iterates all declarations in the translation unit, collecting functions with
   * definitions. Each is harnessed according to its signature: primitive-param
   * functions get {@code __VERIFIER_nondet_*} arguments, {@code main} is
   * delegated to {@code genMainHarness}, and unsupported/variadic functions are
   * skipped.
   *
   * @param Context The AST context for the translation unit being transformed.
   */
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  /**
   * @brief Builds the harness body that invokes the renamed {@code original_main}.
   *
   * Unlike an arbitrary function, {@code main}'s pointer params have a known
   * contract, so instead of skipping it we synthesize a realistic call: a nondet,
   * bounded {@code argc} from {@code __HAVOC_ARGC()} and a matching havocked
   * {@code argv} from {@code __havoc_argv_fill()}, both helpers defined in
   * argv_c_harness.h.
   *
   * @param mainFn The original {@code main} FunctionDecl (already renamed in
   *               the rewriter output).
   * @return C statements (indented, newline-terminated) to splice into the
   *         generated {@code main} body.
   */
  std::string genMainHarness(const clang::FunctionDecl *mainFn);

  std::shared_ptr<std::set<std::string>> _NoOpFunctions;
  clang::Rewriter &_Rewriter;
  HavocBounds _Havoc;
};
