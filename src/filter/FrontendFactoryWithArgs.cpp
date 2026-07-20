// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "FrontendFactoryWithArgs.hpp"
#include "FilterAction.hpp"

#include <clang/Frontend/FrontendAction.h>
#include <memory>

FrontendFactoryWithArgs::FrontendFactoryWithArgs(
    std::map<std::string, std::pair<int, int>> *complexityConfig,
    std::map<std::string, FeatureGate> *featureConfig, llvm::raw_fd_ostream &output)
    : _ComplexityConfig(complexityConfig), _FeatureConfig(featureConfig), _Output(output) {}

std::unique_ptr<clang::FrontendAction> FrontendFactoryWithArgs::create() {
  return std::make_unique<FilterAction>(_ComplexityConfig, _FeatureConfig, _Output);
}
