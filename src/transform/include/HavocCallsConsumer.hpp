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
 * (e.g. a void wrapper whose only calls were themselves dropped) is stripped
 * to a bare declaration and its name recorded in {@code noOpFunctions}, so
 * {@code MainGenConsumer} does not bother harnessing it.
 */
class HavocCallsConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param noOpFunctions   Output set; names of functions whose bodies
   *        collapsed entirely to no-ops are inserted here.
   * @param neededFwdDecls  Output set; file-scope forward declarations a
   *        havocked pointer's prototype-scope struct tag needs, shared with
   *        {@code MainGenConsumer} which emits them into the file prelude.
   * @param rewriter        Shared rewriter for modifying the source buffer.
   */
  HavocCallsConsumer(std::shared_ptr<std::set<std::string>> noOpFunctions,
                     std::shared_ptr<std::set<std::string>> neededFwdDecls,
                     clang::Rewriter &rewriter);

  /**
   * @brief Launches {@code HavocCallsVisitor} and strips any function whose
   * body ended up entirely a no-op.
   *
   * @param Context The AST context for the translation unit being transformed.
   */
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _NoOpFunctions;
  std::shared_ptr<std::set<std::string>> _NeededFwdDecls;
  clang::Rewriter &_Rewriter;
};
