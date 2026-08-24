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
#include <clang/Lex/Lexer.h>
#include <llvm/Support/Casting.h>

HarnessRepairConsumer::HarnessRepairConsumer(clang::Rewriter &rewriter,
                                             std::shared_ptr<std::vector<std::string>> toRemove)
    : _Rewriter(rewriter), _ToRemove(toRemove) {}

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

  // Every harness call is a top-level statement of main's body
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
      // Erase the whole line -- indent, call, semicolon, trailing newline
      clang::SourceLocation begin = call->getBeginLoc();
      clang::SourceLocation lineStart =
          mgr.translateLineCol(mgr.getFileID(begin), mgr.getSpellingLineNumber(begin), 1);
      clang::SourceLocation afterSemi = clang::Lexer::findLocationAfterToken(
          call->getEndLoc(), clang::tok::semi, mgr, context.getLangOpts(),
          /*SkipTrailingWhitespaceAndNewLine=*/true);
      _Rewriter.RemoveText(clang::CharSourceRange::getCharRange(lineStart, afterSemi));
      debugLog(2, "[verify] unharnessed (failed post-transform re-check): " + name);
    }
  }
}
