// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

/**
 * @brief ASTConsumer that havocs every in-file function call.
 *
 * Wraps {@code HavocCallsVisitor} and drives the traversal. After the visitor
 * runs, every call to a function declared in this file has been replaced with
 * a nondeterministic value of the appropriate return type, making each function
 * body intraprocedural.
 *
 * After traversal, any function whose body collapsed entirely to no-ops
 * (e.g. a void wrapper whose only calls were themselves dropped), or that
 * contains a call {@code HavocCallsVisitor} could not havoc (aggregate
 * return, or a non-viable pointer return — {@code Mode::Reject}), is
 * stripped to a bare declaration and its name recorded in
 * {@code discardedFunctions}, so {@code MainGenConsumer} does not bother
 * harnessing it. The latter case matters because the callee may have no
 * definition anywhere in the output (a header-closure prototype, or a
 * filter-rejected sibling): leaving its call in place would compile clean
 * under {@code -fsyntax-only} but call into nothing at runtime.
 */
class HavocCallsConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param discardedFunctions Output set; names of functions stripped to a
   *        bare declaration (no-op body, or an unhavockable call) are inserted here.
   * @param neededFwdDecls  Output set; file-scope forward declarations a
   *        havocked pointer's prototype-scope struct tag needs, shared with
   *        {@code MainGenConsumer} which emits them into the file prelude.
   * @param rewriter        Shared rewriter for modifying the source buffer.
   */
  HavocCallsConsumer(std::shared_ptr<std::set<std::string>> discardedFunctions,
                     std::shared_ptr<std::set<std::string>> neededFwdDecls,
                     clang::Rewriter &rewriter);

  /**
   * @brief Launches {@code HavocCallsVisitor} and strips any function that
   * collapsed to no-ops or contains an unhavockable call.
   *
   * @param Context The AST context for the translation unit being transformed.
   */
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _DiscardedFunctions;
  std::shared_ptr<std::set<std::string>> _NeededFwdDecls;
  clang::Rewriter &_Rewriter;
};
