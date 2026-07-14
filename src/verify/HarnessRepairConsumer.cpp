// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/HarnessRepairConsumer.hpp"

#include "DebugLog.hpp"
#include "VerifierNames.hpp"

#include <algorithm>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>

HarnessRepairConsumer::HarnessRepairConsumer(clang::Rewriter &rewriter,
                                             std::shared_ptr<std::vector<std::string>> toRemove,
                                             std::shared_ptr<VerifyResult> result)
    : _Rewriter(rewriter), _ToRemove(toRemove), _Result(result) {}

void HarnessRepairConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  clang::SourceManager &mgr = context.getSourceManager();

  // Find the generated main. MainGenConsumer guarantees exactly one.
  const clang::FunctionDecl *mainDecl = nullptr;
  for (clang::Decl *decl : context.getTranslationUnitDecl()->decls()) {
    const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (func && func->isMain() && func->doesThisDeclarationHaveABody() &&
        mgr.isInMainFile(func->getLocation())) {
      mainDecl = func;
      break;
    }
  }
  if (!mainDecl)
    return;

  // Every harness call is a top-level statement of main's body (the
  // original_main path also nests __havoc_cstring inside a for loop, but
  // that's a generated helper, exempt either way).
  const auto *body = llvm::dyn_cast<clang::CompoundStmt>(mainDecl->getBody());
  if (!body)
    return;
  for (const clang::Stmt *child : body->body()) {
    const auto *call = llvm::dyn_cast<clang::CallExpr>(child);
    if (!call)
      continue;
    const clang::FunctionDecl *callee = call->getDirectCallee();
    if (!callee)
      continue;
    std::string name = callee->getNameAsString();
    if (isVerifierGenerated(name) || name == "abort")
      continue;
    if (std::find(_ToRemove->begin(), _ToRemove->end(), name) != _ToRemove->end()) {
      // Erasing just the call expression leaves the trailing semicolon as an
      // empty statement, same as HavocCallsVisitor's void-call drops.
      _Rewriter.ReplaceText(call->getSourceRange(), "");
      _Result->removedCalls++;
      debugLog(1, "[verify] unharnessed (failed post-transform re-check): " + name);
    } else {
      _Result->harnessCalls++;
    }
  }
}
