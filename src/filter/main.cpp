// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Filterer.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include <filesystem>
#include <iostream>
#include <optional>

int main(int argc, char **argv) {
  checkClangVersion();
  std::optional<CliInvocation> invocation = parseCliArgs(argc, argv);
  if (!invocation) {
    printUsage("filter");
    return 1;
  }
  // An arg that names nothing on disk was classified as a config; if it
  // doesn't exist either, it's most likely a mistyped input path.
  if (!invocation->configFile.empty() && !std::filesystem::exists(invocation->configFile)) {
    std::cerr << "No such file or directory: " << invocation->configFile << std::endl;
    return 1;
  }

  Filterer filter(invocation->configFile, invocation->inputPath);
  if (!std::filesystem::exists(filter.getDatabaseDir())) {
    std::cerr << "Database directory not found: " << filter.getDatabaseDir() << std::endl;
    return 1;
  }

  filter.run();
  return 0;
}
