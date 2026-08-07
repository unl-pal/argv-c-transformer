// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Filterer.hpp"
#include "FilterAction.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include "DebugLog.hpp"

#include <clang/AST/Type.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

const std::string defaultDatabaseDir = "repos";

Filterer::Filterer(std::string configFile, std::string inputPath) {
  configuration.databaseDir = defaultDatabaseDir;
  if (!configFile.empty())
    parseConfigFile(configFile);
  if (configuration.filterDir.empty())
    configuration.filterDir = inputBaseName(configuration.databaseDir) + "-filtered";
  if (!inputPath.empty()) {
    configuration.databaseDir = inputPath;
    configuration.filterDir = inputBaseName(inputPath) + "-filtered";
  }
};

void Filterer::parseConfigFile(std::string configFile) {
  config = parsePipelineConfig(configFile);

  if (!config.databaseDir.empty()) {
    configuration.databaseDir = config.databaseDir;
    if (!std::filesystem::exists(config.databaseDir))
      debugLog(0, "Database directory not found: " + config.databaseDir);
  }
  if (!config.filterDir.empty())
    configuration.filterDir = config.filterDir;

  globalDebugLevel() = config.fileSettings.at("debugLevel");

  if (globalDebugLevel() >= 1) {
    std::string dump = "[filter] loaded config: " + configFile;
    for (const auto &[k, v] : config.fileSettings)
      dump += "\n  " + k + " = " + std::to_string(v);
    for (const auto &[k, range] : config.complexity)
      dump += "\n  " + k + " = " + std::to_string(range.first) + "," + std::to_string(range.second);
    for (const auto &[k, gate] : config.features)
      dump += "\n  " + k + " = " +
              (gate == FeatureGate::Require   ? "require"
               : gate == FeatureGate::Forbid ? "forbid"
                                              : "ignore");
    debugLog(1, dump);
  }
}

bool Filterer::checkPotentialFile(std::string fileName) {
  std::ifstream file(fileName);
  if (!file.is_open()) {
    debugLog(0, "Failed to open file: " + fileName);
    return false;
  }

  std::string line;
  int count = 0;
  while (std::getline(file, line)) {
    if (!line.empty())
      count++;
  }

  if (count < config.fileSettings.at("minFileLoC")) {
    debugLog(1, "[filter] skipped (LoC " + std::to_string(count) + " < minFileLoC " +
                    std::to_string(config.fileSettings.at("minFileLoC")) + "): " + fileName);
    return false;
  }
  if (count > config.fileSettings.at("maxFileLoC")) {
    debugLog(1, "[filter] skipped (LoC " + std::to_string(count) + " > maxFileLoC " +
                    std::to_string(config.fileSettings.at("maxFileLoC")) + "): " + fileName);
    return false;
  }
  return true;
}

int Filterer::getAllCFiles(std::filesystem::path pathObject,
                           std::vector<std::string> &filesToFilter, int numFiles) {
  if (!std::filesystem::exists(pathObject)) {
    debugLog(1, "[filter] path does not exist: " + pathObject.string());
    return 0;
  }
  if (std::filesystem::is_regular_file(pathObject)) {
    if (pathObject.has_extension()) {
      if (pathObject.extension() == ".c") {
        debugLog(3, "[filter] queued: " + pathObject.filename().string());
        filesToFilter.push_back(pathObject.string());
        return 1;
      } else {
        debugLog(3, "[filter] skipped (not .c): " + pathObject.filename().string());
        return 0;
      }
    } else {
      debugLog(3, "[filter] skipped (no extension): " + pathObject.filename().string());
      return 0;
    }
  } else if (std::filesystem::is_directory(pathObject)) {
    // Don't let filepath issues break loop, log and move on with
    // whatever files were already queued.
    try {
      for (const std::filesystem::directory_entry &entry :
           std::filesystem::directory_iterator(pathObject)) {
        numFiles += getAllCFiles(entry.path(), filesToFilter);
      }
    } catch (const std::filesystem::filesystem_error &e) {
      debugLog(1, "[filter] skipping unreadable directory " + pathObject.string() +
                      ": " + e.what());
    }
    return numFiles;
  } else {
    debugLog(3, "[filter] ignored: " + pathObject.filename().string());
    return 0;
  }
  return 0;
}

int Filterer::run() {
  std::filesystem::path pathObject(configuration.databaseDir);
  std::vector<std::string> filesToFilter;

  debugLog(1, "[filter] scanning: " + pathObject.string());

  int filesFound = getAllCFiles(pathObject, filesToFilter, 0);
  debugLog(1, "[filter] found " + std::to_string(filesFound) + " .c file(s)");

  int passed = 0;
  int i = 0;
  for (const std::string &fileName : filesToFilter) {
    std::cout << "\r[filter] " << ++i << "/" << filesFound << std::flush;
    debugLog(1, "[filter] file: " + fileName);
    if (!checkPotentialFile(fileName))
      continue;
    passed++;

    std::filesystem::path oldPath(fileName);
    // Mirror the input's path under filterDir by stripping the databaseDir
    // prefix; relative() handles absolute and relative inputs uniformly.
    // relative() yields "." when the input path IS the file (single-file mode).
    std::filesystem::path relPath = std::filesystem::relative(oldPath, configuration.databaseDir);
    if (relPath.empty() || *relPath.begin() == ".." || relPath == ".")
      relPath = oldPath.filename();
    std::filesystem::path newPath = std::filesystem::path(configuration.filterDir) / relPath;

    // Hard guard: never write over the source, whatever the path arithmetic.
    if (std::filesystem::weakly_canonical(oldPath) == std::filesystem::weakly_canonical(newPath)) {
      debugLog(0, "Refusing to overwrite source file: " + oldPath.string());
      continue;
    }

    std::filesystem::create_directories(newPath.parent_path());
    std::error_code ec;
    llvm::raw_fd_ostream output(llvm::StringRef(newPath.string()), ec);
    if (ec) {
      debugLog(0, "Cannot open output file " + newPath.string() + ": " + ec.message());
      continue;
    }

    FrontendFactoryWithArgs factory(&config.complexity, &config.features, output);
    bool ran = runToolOnFile(oldPath.string(), factory);
    output.close();
    if (!ran) {
      debugLog(1, "[filter] clang tool failed on: " + oldPath.string());
      continue;
    }

  }
  if (filesFound > 0)
    std::cout << std::endl;

  std::cout << "\n=== Filter summary ===\n"
            << "  Files found:            " << filesFound << "\n"
            << "  Passed pre-filter:      " << passed << "\n"
            << "  Skipped:                " << (filesFound - passed) << std::endl;
  return 0;
}
