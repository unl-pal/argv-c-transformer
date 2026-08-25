// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "HavocCallsConsumer.hpp"
#include "HavocCallsVisitor.hpp"

#include "DebugLog.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/Casting.h>

HavocCallsConsumer::HavocCallsConsumer(std::shared_ptr<std::set<std::string>> noOpFunctions,
                                       std::shared_ptr<std::set<std::string>> neededFwdDecls,
                                       clang::Rewriter &rewriter)
    : _NoOpFunctions(noOpFunctions), _NeededFwdDecls(neededFwdDecls), _Rewriter(rewriter) {}

void HavocCallsConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  HavocCallsVisitor Visitor(&Context, _NeededFwdDecls, _Rewriter);
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());

  // Strip any function whose body collapsed entirely to no-ops
  clang::SourceManager &mgr = Context.getSourceManager();
  for (clang::Decl *decl : Context.getTranslationUnitDecl()->decls()) {
    const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (!func || !mgr.isInMainFile(func->getLocation()))
      continue;
    if (!func->isThisDeclarationADefinition() || func->getLocation().isMacroID())
      continue;
    if (!Visitor.isNoOp(func->getBody()))
      continue;
    debugLog(2, "[transform] " + func->getNameAsString() + " body collapsed entirely to no-ops");
    clang::SourceRange bodyRange = func->getBody()->getSourceRange();
    if (bodyRange.isValid())
      _Rewriter.ReplaceText(bodyRange, ";");
    _NoOpFunctions->insert(func->getNameAsString());
  }
}
