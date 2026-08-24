// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Verifier.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include <filesystem>
#include <iostream>
#include <optional>

int main(int argc, char **argv) {
  checkClangVersion();
  checkRuntimeClangVersion();
  std::optional<CliInvocation> invocation = parseCliArgs(argc, argv);
  if (!invocation) {
    printUsage("verify");
    return 1;
  }
  if (!invocation->configFile.empty() && !std::filesystem::exists(invocation->configFile)) {
    std::cerr << "No such file or directory: " << invocation->configFile << std::endl;
    return 1;
  }

  Verifier verifier(invocation->configFile, invocation->inputPath);
  if (!std::filesystem::exists(verifier.getTransformDir())) {
    std::cerr << "Transform directory not found: " << verifier.getTransformDir() << std::endl;
    return 1;
  }

  verifier.run();
  return 0;
}
