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
  if (!std::filesystem::exists(filter.getDatabaseDir())) {
    std::cerr << "Database directory not found: " << filter.getDatabaseDir() << std::endl;
    return 1;
  }

  // When an input was given on the command line, the transform must read the
  // filter's resolved output directory, not the input itself.
  std::string transformInput;
  if (!invocation->inputPath.empty()) transformInput = filter.getFilterDir();
  Transformer transformer(invocation->configFile, transformInput);
  // filter's resolved input tree, so transform can resolve local #includes
  // against it too (filterDir only mirrors .c files, not headers).
  transformer.setDatabaseDir(filter.getDatabaseDir());

  // Verify always reads the transform's resolved output directory, whichever
  // of default / config / derived-from-input won.
  Verifier verifier(invocation->configFile, transformer.getTransformDir());

  // fail fast if any output dir overlaps another pipeline dir, or already exists and is not empty
  const std::vector<std::string> outputDirs = {filter.getFilterDir(), transformer.getTransformDir(),
                                               verifier.getBenchmarkDir()};
  for (size_t i = 0; i < outputDirs.size(); i++) {
    std::vector<std::string> others = {filter.getDatabaseDir()};
    for (size_t j = 0; j < outputDirs.size(); j++)
      if (j != i) others.push_back(outputDirs[j]);
    checkOutputDirOverlap(outputDirs[i], others);
  }
  PipelineConfig rawConfig = parsePipelineConfig(invocation->configFile);
  bool cleanOutput = rawConfig.fileSettings.at("cleanOutput") != 0;
  if (!cleanOutput) {
    for (const std::string &dir : outputDirs) {
      std::filesystem::path path(dir);
      if (std::filesystem::exists(path) && !std::filesystem::is_empty(path)) {
        std::cerr << "argv-c: output directory '" << dir << "' already exists and is not empty.\n"
                   << "Set cleanOutput=true in the config to wipe it first, or remove it manually."
                   << std::endl;
        return 1;
      }
    }
  }

  filter.run();
  transformer.run();
  verifier.run();

  // cleanup if user didn't specify intermediate dirs
  if (rawConfig.filterDir.empty() && rawConfig.transformDir.empty()) {
    std::error_code ec;
    std::filesystem::remove_all(filter.getFilterDir(), ec);
    std::filesystem::remove_all(transformer.getTransformDir(), ec);
  }
  return 0;
}
