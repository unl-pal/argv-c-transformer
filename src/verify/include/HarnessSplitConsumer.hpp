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

/** @brief One harness block of the generated main and the function it calls. */
struct HarnessEntry {
  std::string target;
  std::string block; ///< Whole lines, indent through trailing newline.
};

/**
 * @brief A verified file cut around its harness blocks. Every benchmark is
 * {@code head + block + tail} for one entry; {@code head + all blocks + tail}
 * is the whole file.
 */
struct HarnessSplit {
  std::string head;
  std::string tail;
  std::vector<HarnessEntry> entries;
};

/**
 * @brief Splits the generated main by harnessed function.
 *
 * MainGenConsumer emits each harness call and its setup as a top-level block
 * of main, ending in the call. Each block is turned into a HarnessEntry for
 * splitting into seperate files with the same head and tail.
 */
class HarnessSplitConsumer : public clang::ASTConsumer {
public:
  HarnessSplitConsumer(clang::Rewriter &rewriter,
                       std::shared_ptr<std::vector<std::string>> toRemove, HarnessSplit &output);

  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  clang::Rewriter &_Rewriter;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
  HarnessSplit &_Output;
};
