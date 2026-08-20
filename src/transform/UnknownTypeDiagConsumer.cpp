// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "UnknownTypeDiagConsumer.hpp"
#include "DebugLog.hpp"

#include <clang/AST/DeclarationName.h>
#include <clang/Basic/DiagnosticSema.h>
#include <clang/Basic/IdentifierTable.h>

UnknownTypeDiagConsumer::UnknownTypeDiagConsumer(
    std::shared_ptr<std::set<std::string>> unresolvedTypeNames)
    : _UnresolvedTypeNames(unresolvedTypeNames) {
}

void UnknownTypeDiagConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level,
                                               const clang::Diagnostic &Info) {
  unsigned id = Info.getID();
  bool isUnknownTypename =
      id == clang::diag::err_unknown_typename || id == clang::diag::err_unknown_typename_suggest;
  bool isUndeclaredVarUse =
      id == clang::diag::err_undeclared_var_use || id == clang::diag::err_undeclared_var_use_suggest;
  if (!isUnknownTypename && !isUndeclaredVarUse)
    return;

  // Clang does not guarantee a diagnostic ID always carries its name
  // argument with the same encoding: err_undeclared_var_use_suggest (and
  // presumably its siblings) has been observed emitting the same identifier
  // as an IdentifierInfo* in one call and a DeclarationName in another,
  // for the same diagnostic ID, in the same TU. Handle both. Recording
  // nothing for a genuinely unrecognized encoding is safe: at worst
  // AddStdIncludesConsumer misses one header, which checkCompilable()
  // catches downstream -- never worth aborting the whole file over.
  switch (Info.getArgKind(0)) {
  case clang::DiagnosticsEngine::ak_identifierinfo:
    _UnresolvedTypeNames->insert(Info.getArgIdentifier(0)->getName().str());
    break;
  case clang::DiagnosticsEngine::ak_declarationname:
    _UnresolvedTypeNames->insert(
        clang::DeclarationName::getFromOpaqueInteger(Info.getRawArg(0)).getAsString());
    break;
  default:
    debugLog(2, "UnknownTypeDiagConsumer: diagnostic " + std::to_string(id) +
                    " arg0 has unrecognized kind " + std::to_string(Info.getArgKind(0)) +
                    "; skipping");
    break;
  }
}
