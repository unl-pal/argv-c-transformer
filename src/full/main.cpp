// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "Filterer.hpp"
#include "Transformer.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

int main(int argc, char **argv) {
  checkClangVersion();
  std::optional<CliInvocation> invocation = parseCliArgs(argc, argv);
  if (!invocation) {
    printUsage("full");
    return 1;
  }
  if (!invocation->configFile.empty() && !std::filesystem::exists(invocation->configFile)) {
    std::cerr << "No such file or directory: " << invocation->configFile << std::endl;
    return 1;
  }

  Filterer filter(invocation->configFile, invocation->inputPath);
  filter.run();

  // When an input was given on the command line, the transform must read the
  // filter's resolved output directory, not the input itself.
  std::string transformInput;
  if (!invocation->inputPath.empty())
    transformInput = filter.getFilterDir();
  Transformer transformer(invocation->configFile, transformInput);
  transformer.run();
  return 0;
}
