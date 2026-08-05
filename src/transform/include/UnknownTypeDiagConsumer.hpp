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
 * An unresolved type name causes Sema to drop the whole statement. Instead, we catch
 * the diagnostic and record the identifier, checking it against our StdHeaders map
 * so that we can include the necessary header if necessary.
 */
class UnknownTypeDiagConsumer : public clang::DiagnosticConsumer {
public:
  /**
   * @brief Constructs the consumer, binding the shared output set.
   *
   * @param unresolvedTypeNames Populated w/ every unresolved type identifier.
   *        Read by AddStdIncludesConsumer.
   */
  explicit UnknownTypeDiagConsumer(std::shared_ptr<std::set<std::string>> unresolvedTypeNames);

  /**
   * @brief Records the identifier of an unresolved-type diagnostic.
   *
   * @param DiagLevel Severity of the diagnostic (unused - matched by ID only).
   * @param Info      The diagnostic being reported, including its format arguments.
   */
  void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel,
                        const clang::Diagnostic &Info) override;

private:
  std::shared_ptr<std::set<std::string>> _UnresolvedTypeNames;
};
