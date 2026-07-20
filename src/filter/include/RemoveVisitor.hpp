// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Visitor that strips the bodies of filtered-out functions.
 *
 * {@code VisitFunctionDecl} replaces the {@code { ... }} body of each
 * function in {@code _ToRemove} with {@code ;}, turning the definition into
 * a bare declaration. The signature is left intact so that later transform
 * passes (e.g. {@code HavocCallsVisitor}) still see the real return type for
 * any remaining calls to it, instead of Clang's implicit-int fallback for an
 * undeclared function.
 */
class RemoveVisitor : public clang::RecursiveASTVisitor<RemoveVisitor> {
public:
  /**
   * @brief Constructs the visitor with the shared pipeline state.
   *
   * @param rewriter     Shared rewriter; body replacements accumulate here.
   * @param toRemove     Names of functions to strip, from {@code FilterFunctionsConsumer}.
   */
  RemoveVisitor(clang::Rewriter &rewriter, std::shared_ptr<std::vector<std::string>> toRemove);

  /**
   * @brief Replaces the body of each function in {@code _ToRemove} with {@code ;}.
   *
   * Skips macro-expanded locations (not writable by the Rewriter) and
   * declarations with no body (already bare prototypes).
   */
  bool VisitFunctionDecl(clang::FunctionDecl *D);

  /** @brief Uses pre-order traversal (default); post-order left as future option. */
  bool shouldTraversePostOrder();

private:
  clang::SourceManager &_Mgr;
  clang::Rewriter &_Rewriter;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
};
