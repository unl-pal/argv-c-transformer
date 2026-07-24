// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/Basic/Diagnostic.h>
#include <memory>
#include <set>
#include <string>

/**
 * @brief DiagnosticConsumer that recovers unresolved standard type names lost to the AST.
 *
 * An unresolved type name in a local variable declaration causes Sema to drop
 * the whole DeclStmt. The identifier is only ever
 * observable in the diagnostic Clang emits while parsing.
 *
 * Two diagnostic families report this:
 *  - `err_unknown_typename` (+ `_suggest`): the declaration is unambiguous
 *    so Sema reports the unresolved type directly. Argument 0 is an `IdentifierInfo*`.
 *  - `err_undeclared_var_use` (+ `_suggest`): a *bare* local declaration
 *    is syntactically ambiguous, and Clang's heuristic guesses.
 *    Argument 0 here is a `DeclarationName`.
 * Both are matched by name only (`StdHeaders.hpp` lookup)
 *
 * Every other diagnostic is swallowed
 */
class UnknownTypeDiagConsumer : public clang::DiagnosticConsumer {
public:
  /**
   * @brief Constructs the consumer, binding the shared output set.
   *
   * @param unresolvedTypeNames Populated with the spelled name of every
   *        unresolved type identifier seen; read by AddStdIncludesConsumer
   *        once parsing completes.
   */
  explicit UnknownTypeDiagConsumer(std::shared_ptr<std::set<std::string>> unresolvedTypeNames);

  /**
   * @brief Records the identifier of an unresolved-type diagnostic; ignores everything else.
   *
   * @param DiagLevel Severity of the diagnostic (unused - matched by ID only).
   * @param Info      The diagnostic being reported, including its format arguments.
   */
  void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel,
                        const clang::Diagnostic &Info) override;

private:
  std::shared_ptr<std::set<std::string>> _UnresolvedTypeNames;
};
