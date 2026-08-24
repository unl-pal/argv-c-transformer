// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Keeps the generated main consistent with the verify stage's
 * removals.
 *
 * RemoveConsumer strips a rejected function's body to {@code ;}. This
 * consumer erases the entire call statement (indent, call, semicolon, and
 * trailing newline) from main's body, so a harness with nothing left calling
 * anything is textually indistinguishable from one MainGenConsumer generated
 * empty to begin with -- {@code harnessIsEmpty} catches both.
 */
class HarnessRepairConsumer : public clang::ASTConsumer {
public:
  HarnessRepairConsumer(clang::Rewriter &rewriter,
                        std::shared_ptr<std::vector<std::string>> toRemove);

  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  clang::Rewriter &_Rewriter;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
};
