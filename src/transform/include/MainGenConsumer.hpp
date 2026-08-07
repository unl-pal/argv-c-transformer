// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
   * @param neededSuffixes Verifier suffixes used by the harness, shared with
   *        {@code AddVerifiersConsumer} which emits the extern declarations.
   * @param noOpFunctions  Names of functions {@code HavocCallsConsumer} found
   *        to have an entirely no-op body; these are not harnessed.
   * @param rewriter       Shared rewriter for modifying the source buffer.
   */
  MainGenConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                  std::shared_ptr<std::set<std::string>> noOpFunctions, clang::Rewriter &rewriter);

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
   * @brief A synthesized call: setup statements plus the argument list.
   *
   * Parameters, unlike havocked return values, are in statement position, so
   * they can be preceded by setup. That is what makes bounded integer
   * arguments possible.
   */
  struct HarnessCall {
    std::string prologue; ///< Indented, newline-terminated statements before the call.
    std::string args;     ///< Comma-separated argument expressions.
    bool viable = false;  ///< False means the function cannot be harnessed at all.
  };

  /**
   * @brief Synthesizes arguments for one call to an arbitrary function.
   *
   * Primitive parameters become {@code __VERIFIER_nondet_*()} calls; pointer
   * parameters are classified by {@code planPointer} and rendered by
   * {@code renderPointerStorage} as stack storage filled with
   * {@code __VERIFIER_nondet_memory}. When a pointer is present, integer
   * parameters are emitted as locals clamped to {@code __HAVOC_ARRAY_ELEMS}
   * instead, so that any index derived from them stays inside the block handed
   * to the callee.
   *
   * @param func    The function to synthesize a call for.
   * @param Context The AST context, for the SourceManager (telling types that
   *                survive into the output from those defined only in a stripped
   *                header) and the PrintingPolicy (spelling storage declarations).
   * @return The call; check {@code viable} before using it.
   */
  HarnessCall genCallHarness(const clang::FunctionDecl *func, clang::ASTContext &Context);

  /**
   * @brief Builds the harness body that invokes the renamed {@code original_main}.
   *
   * Unlike an arbitrary function, {@code main}'s pointer params have a known
   * contract, so instead of skipping it we synthesize a realistic call: a nondet
   * {@code argc} bounded to [{@code __HAVOC_ARGC_MIN}, {@code __HAVOC_ARGC_MAX}]
   * via {@code abort()}, and a fixed-extent {@code argv} array of havocked,
   * null-terminated C strings backed by a stack buffer (filled with
   * {@code __VERIFIER_nondet_memory}, no heap). The bounds come from
   * {@code HavocPolicy.hpp} and are emitted as macros by
   * {@code AddVerifiersConsumer}. Registers any verifier helpers it uses in
   * {@code _NeededSuffixes}.
   *
   * @param mainFn The original {@code main} FunctionDecl (already renamed in
   *               the rewriter output).
   * @return C statements (indented, newline-terminated) to splice into the
   *         generated {@code main} body.
   */
  std::string genMainHarness(const clang::FunctionDecl *mainFn);

  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  std::shared_ptr<std::set<std::string>> _NoOpFunctions;
  clang::Rewriter &_Rewriter;
  /// Runs across every synthesized call, since all the locals they declare
  /// share the generated main's single scope and so must not collide.
  unsigned _LocalCounter = 0;
};
