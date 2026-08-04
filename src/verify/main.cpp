// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Verifier.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include <filesystem>
#include <iostream>
#include <optional>

// Target for calling the Verifier individually
int main(int argc, char **argv) {
  checkClangVersion();
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
  verifier.run();
  return 0;
}
