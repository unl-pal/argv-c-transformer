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

HavocCallsConsumer::HavocCallsConsumer(std::shared_ptr<std::set<std::string>> discardedFunctions,
                                       std::shared_ptr<std::set<std::string>> neededFwdDecls,
                                       clang::Rewriter &rewriter)
    : _DiscardedFunctions(discardedFunctions), _NeededFwdDecls(neededFwdDecls), _Rewriter(rewriter) {}

void HavocCallsConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  HavocCallsVisitor Visitor(&Context, _NeededFwdDecls, _Rewriter);
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());

  // Strip any function whose body collapsed entirely to no-ops, or that
  // contains a call the visitor could not havoc (its callee may have no
  // definition anywhere in the output).
  clang::SourceManager &mgr = Context.getSourceManager();
  for (clang::Decl *decl : Context.getTranslationUnitDecl()->decls()) {
    const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (!func || !mgr.isInMainFile(func->getLocation()))
      continue;
    if (!func->isThisDeclarationADefinition() || func->getLocation().isMacroID())
      continue;
    bool tainted = Visitor.tainted().count(func) != 0;
    if (!tainted && !Visitor.isNoOp(func->getBody()))
      continue;
    debugLog(2, "[transform] " + func->getNameAsString() +
                    (tainted ? " contains an unhavockable call; discarded"
                             : " body collapsed entirely to no-ops"));
    clang::SourceRange bodyRange = func->getBody()->getSourceRange();
    if (bodyRange.isValid())
      _Rewriter.ReplaceText(bodyRange, ";");
    _DiscardedFunctions->insert(func->getNameAsString());
  }
}
