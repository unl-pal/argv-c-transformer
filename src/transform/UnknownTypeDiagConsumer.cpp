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

  // Guarded against a future Clang version diagnostic change
  if (isUnknownTypename) {
    if (Info.getArgKind(0) != clang::DiagnosticsEngine::ak_identifierinfo){
      debugLog(0, "ERROR: Potential Clang Version Issue in UnknownTypeDiag recovery");
      exit(-1);
    }
    _UnresolvedTypeNames->insert(Info.getArgIdentifier(0)->getName().str());
  } else {
    if (Info.getArgKind(0) != clang::DiagnosticsEngine::ak_declarationname) {
      debugLog(0, "ERROR: Potential Clang Version Issue in UnknownTypeDiag recovery");
      exit(-1);
    }
    clang::DeclarationName name = clang::DeclarationName::getFromOpaqueInteger(Info.getRawArg(0));
    _UnresolvedTypeNames->insert(name.getAsString());
  }
}
