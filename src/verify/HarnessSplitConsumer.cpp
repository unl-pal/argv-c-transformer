// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/HarnessSplitConsumer.hpp"

#include "DebugLog.hpp"

#include <algorithm>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>

HarnessSplitConsumer::HarnessSplitConsumer(clang::Rewriter &rewriter,
                                           std::shared_ptr<std::vector<std::string>> toRemove,
                                           HarnessSplit &output)
    : _Rewriter(rewriter), _ToRemove(toRemove), _Output(output) {}

void HarnessSplitConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  clang::SourceManager &mgr = context.getSourceManager();
  clang::FileID fid = mgr.getMainFileID();

  const clang::FunctionDecl *mainDecl = nullptr;
  for (clang::Decl *decl : context.getTranslationUnitDecl()->decls()) {
    const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (func && func->isMain() && func->doesThisDeclarationHaveABody() &&
        mgr.isInMainFile(func->getLocation())) {
      mainDecl = func;
      break;
    }
  }
  if (!mainDecl) return;
  const auto *body = llvm::dyn_cast<clang::CompoundStmt>(mainDecl->getBody());
  if (!body) return;

  auto lineStart = [&](clang::SourceLocation loc, unsigned offset = 0) {
    return mgr.translateLineCol(fid, mgr.getSpellingLineNumber(loc) + offset, 1);
  };
  auto rewritten = [&](clang::SourceLocation begin, clang::SourceLocation end) {
    return _Rewriter.getRewrittenText(clang::CharSourceRange::getCharRange(begin, end));
  };

  const clang::CompoundStmt *first = nullptr;
  const clang::CompoundStmt *last = nullptr;
  for (const clang::Stmt *child : body->body()) {
    const auto *block = llvm::dyn_cast<clang::CompoundStmt>(child);
    if (!block || block->body_empty()) continue;
    if (!first) first = block;
    last = block;

    const auto *call = llvm::dyn_cast<clang::CallExpr>(block->body_back());
    const clang::FunctionDecl *callee = call ? call->getDirectCallee() : nullptr;
    if (!callee) continue;
    std::string name = callee->getNameAsString();
    if (std::find(_ToRemove->begin(), _ToRemove->end(), name) != _ToRemove->end()) {
      debugLog(2, "[verify] unharnessed (failed post-transform re-check): " + name);
      continue;
    }
    _Output.entries.push_back(
        {name, rewritten(lineStart(block->getBeginLoc()), lineStart(block->getRBracLoc(), 1))});
  }
  if (!first) return;

  _Output.head = rewritten(mgr.getLocForStartOfFile(fid), lineStart(first->getBeginLoc()));
  _Output.tail = rewritten(lineStart(last->getRBracLoc(), 1), mgr.getLocForEndOfFile(fid));
}
