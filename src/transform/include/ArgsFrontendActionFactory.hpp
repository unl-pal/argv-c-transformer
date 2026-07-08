// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

/**
 * @brief Carries the output stream into Clang's tool runner.
 *
 * Clang's {@code ClangTool::run()} only knows how to call {@code create()} on
 * a {@code FrontendActionFactory}. This subclass stores the output stream so
 * that each {@code TransformAction} it creates can write the rewritten source
 * without that stream being a global.
 */
class ArgsFrontendFactory : public clang::tooling::FrontendActionFactory {
public:
  /**
   * @brief Constructs the factory, binding the output stream.
   *
   * @param output Reference to the output stream for the transformed file.
   */
  ArgsFrontendFactory(llvm::raw_ostream &output);

  /**
   * @brief Called by {@code ClangTool} once per source file to create the action.
   *
   * Returns a new {@code TransformAction} loaded with the output stream.
   *
   * @return Owning pointer to the created action.
   */
  std::unique_ptr<clang::FrontendAction> create() override;

private:
  llvm::raw_ostream &_Output;
};
