// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

/**
 * @brief ASTConsumer that re-injects standard headers stripped includes left missing.
 *
 * Walks the AST for used standard types and called standard functions (see
 * {@code TypeCollector} / {@code FunctionCollector} in the .cpp), maps each to
 * its header via {@code StdHeaders.hpp}, and inserts any {@code #include <...>}
 * not already present (tracked in {@code existingIncludes}).
 */
class AddStdIncludesConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param existingIncludes System includes already present in the file (recorded
   *        by {@code IncludeFinder}); used to avoid emitting duplicates.
   * @param unresolvedTypeNames Unresolved type identifiers recovered by
   *        {@code UnknownTypeDiagConsumer} during parsing - names that never
   *        made it into the AST 
   * @param rewriter Shared rewriter for modifying the source buffer.
   */
  AddStdIncludesConsumer(std::shared_ptr<std::set<std::string>> existingIncludes,
                         std::shared_ptr<std::set<std::string>> unresolvedTypeNames,
                         clang::Rewriter &rewriter);

  /**
   * @brief Collects used standard types and inserts the headers they require.
   *
   * @param Context The AST context for the translation unit being transformed.
   */
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  std::shared_ptr<std::set<std::string>> _ExistingIncludes;
  std::shared_ptr<std::set<std::string>> _UnresolvedTypeNames;
  clang::Rewriter &_Rewriter;
};
