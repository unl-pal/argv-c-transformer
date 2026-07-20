// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/RemoveVisitor.hpp"

#include <clang/AST/Stmt.h>

RemoveVisitor::RemoveVisitor(clang::Rewriter &rewriter,
                             std::shared_ptr<std::vector<std::string>> toRemove)
    : _Mgr(rewriter.getSourceMgr()), _Rewriter(rewriter), _ToRemove(toRemove) {}

bool RemoveVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (!D)
    return false;
  if (_Mgr.isInMainFile(D->getLocation())) {
    // Macro-expanded locations are not writable by the Rewriter
    if (D->getLocation().isMacroID())
      return clang::RecursiveASTVisitor<RemoveVisitor>::VisitFunctionDecl(D);

    if (D->doesThisDeclarationHaveABody()) {
      for (const std::string &name : *_ToRemove) {
        if (name == D->getNameAsString()) {
          clang::SourceRange bodyRange = D->getBody()->getSourceRange();
          if (bodyRange.isValid())
            _Rewriter.ReplaceText(bodyRange, ";");
          break;
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveVisitor>::VisitFunctionDecl(D);
}

bool RemoveVisitor::shouldTraversePostOrder() { return false; }
