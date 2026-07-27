// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "Filterer.hpp"
#include "Transformer.hpp"
#include "Verifier.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include "ConfigParser.hpp"
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

int main(int argc, char **argv) {
  checkClangVersion();
  checkRuntimeClangVersion();
  std::optional<CliInvocation> invocation = parseCliArgs(argc, argv);
  if (!invocation) {
    printUsage("argv-c");
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
  // filter's resolved input tree, so transform can resolve local #includes
  // against it too (filterDir only mirrors .c files, not headers).
  transformer.setDatabaseDir(filter.getDatabaseDir());
  transformer.run();

  // Verify always reads the transform's resolved output directory, whichever
  // of default / config / derived-from-input won.
  Verifier verifier(invocation->configFile, transformer.getTransformDir());
  verifier.run();

  // cleanup if user didn't specify intermediate dirs
  PipelineConfig rawConfig = parsePipelineConfig(invocation->configFile);
  if (rawConfig.filterDir.empty() && rawConfig.transformDir.empty()) {
    std::error_code ec;
    std::filesystem::remove_all(filter.getFilterDir(), ec);
    std::filesystem::remove_all(transformer.getTransformDir(), ec);
  }
  return 0;
}
