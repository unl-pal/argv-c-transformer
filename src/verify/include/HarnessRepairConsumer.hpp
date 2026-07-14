// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VerifyResult.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Keeps the generated main consistent with the verify stage's
 * removals, and reports how much harness is left.
 *
 * RemoveConsumer strips a rejected function's body to {@code ;}, which
 * would leave the generated main calling a declared-but-undefined function
 * — that passes {@code -fsyntax-only} but is broken for verification. This
 * consumer erases those call statements from main's body. MainGenConsumer
 * emits every harness call as a top-level statement of main, so only
 * main's direct children need scanning.
 *
 * It also counts the harness calls that remain (ignoring injected
 * {@code __VERIFIER_*}/{@code __havoc_*} helpers and the abort guard), so
 * the driver can discard a benchmark whose harness emptied out entirely.
 */
class HarnessRepairConsumer : public clang::ASTConsumer {
public:
  HarnessRepairConsumer(clang::Rewriter &rewriter,
                        std::shared_ptr<std::vector<std::string>> toRemove,
                        std::shared_ptr<VerifyResult> result);

  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  clang::Rewriter &_Rewriter;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
  std::shared_ptr<VerifyResult> _Result;
};
