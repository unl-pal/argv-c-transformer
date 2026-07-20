// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Filterer.hpp"
#include "FrontendFactoryWithArgs.hpp"
#include "ClangToolUtils.hpp"
#include "DebugLog.hpp"

#include <algorithm>
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
#include <regex>
#include <sstream>
#include <string>

const std::string defaultFilterDir = "filteredFiles";
const std::string defaultDatabaseDir = "database";
/// Not yet implemented in code - currently handled by scripts
const bool defaultWipeOldBenchmarks = true;

Filterer::Filterer(std::string configFile) {
  configuration.filterDir = defaultFilterDir;
  configuration.databaseDir = defaultDatabaseDir;
  configuration.wipeOldBenchmarks = defaultWipeOldBenchmarks;
  parseConfigFile(configFile);
};

void Filterer::parseConfigFile(std::string configFile) {
  if (!std::filesystem::exists(configFile)) {
    std::cerr << "Config file not found: " << configFile << " — using defaults" << std::endl;
    return;
  }
  for (auto [key, value] : parseIniFile(configFile)) {
    if (complexityConfig.count(key)) {
      size_t comma = value.find(',');
      if (comma == std::string::npos) {
        std::cerr << "Warning: complexity key '" << key
                  << "' expects 'min,max' — ignoring value '" << value << "'" << std::endl;
        continue;
      }
      try {
        int min = std::stoi(trim(value.substr(0, comma)));
        int max = std::stoi(trim(value.substr(comma + 1)));
        complexityConfig.at(key) = {min, max};
      } catch (...) {
        std::cerr << "Warning: could not parse complexity range for '" << key << "': '" << value
                  << "'" << std::endl;
      }
    } else if (featureConfig.count(key)) {
      std::string v = trim(value);
      std::transform(v.begin(), v.end(), v.begin(), ::tolower);
      if (v == "require") {
        featureConfig.at(key) = FeatureGate::Require;
      } else if (v == "forbid") {
        featureConfig.at(key) = FeatureGate::Forbid;
      } else if (v == "ignore" || v.empty()) {
        featureConfig.at(key) = FeatureGate::Ignore;
      } else {
        std::cerr << "Warning: unrecognised gate '" << value << "' for feature '" << key
                  << "' — ignoring" << std::endl;
      }
    } else if (fileSettings.count(key)) {
      try {
        fileSettings.at(key) = std::stoi(value);
      } catch (...) {
      }
      if (value == "false" || value == "False") {
        fileSettings.at(key) = 0;
      } else if (value == "true" || value == "True") {
        fileSettings.at(key) = 1;
      }
    } else if (key == "databaseDir") {
      configuration.databaseDir = value;
      if (!std::filesystem::exists(value))
        std::cerr << "Database directory not found: " << value << std::endl;
    } else if (key == "filterDir") {
      configuration.filterDir = value;
      if (!std::filesystem::exists(value))
        std::filesystem::create_directory(value);
    } else if (key == "wipeOldBenchmarks") {
      configuration.wipeOldBenchmarks = parseConfigBool(value);
    } else if (key == "benchmarkDir" || key == "keepCompilesOnly" || key == "csv" ||
               key == "downloadDir" || key == "language" || key == "projectCount" ||
               key == "minNumStars" || key == "minRepoLoc") {
      // consumed by the transform step or Downloader.py; not a filter key
    } else if (key == "debug") {
      // silently ignored: replaced by debugLevel
    } else {
      std::cerr << "Unknown config key: " << key << std::endl;
    }
  }

  globalDebugLevel() = fileSettings.at("debugLevel");

  if (globalDebugLevel() >= 1) {
    std::string dump = "[filter] loaded config: " + configFile;
    for (const auto &[k, v] : fileSettings)
      dump += "\n  " + k + " = " + std::to_string(v);
    for (const auto &[k, range] : complexityConfig)
      dump += "\n  " + k + " = " + std::to_string(range.first) + "," + std::to_string(range.second);
    for (const auto &[k, gate] : featureConfig)
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
    std::cerr << "Failed to open file: " << fileName << std::endl;
    return false;
  }

  std::regex allowedHeadersPattern("#(include|import)\\ *[<\"]([\\w\\/0-9\\.]*)[\">]");
  std::string line;
  std::smatch match;
  int count = 0;
  while (std::getline(file, line)) {
    if (std::regex_search(line, match, allowedHeadersPattern)) {
      bool isStdLib =
          std::find(stdLibNames.begin(), stdLibNames.end(), match[2]) != stdLibNames.end();
      if (!isStdLib && !fileSettings.at("useNonStdHeaders")) {
        debugLog(1, "[filter] skipped (non-std header '" + match[2].str() +
                        "', useNonStdHeaders=" +
                        std::to_string(fileSettings.at("useNonStdHeaders")) + "): " + fileName);
        return false;
      }
    }
    if (!line.empty())
      count++;
  }

  if (count < fileSettings.at("minFileLoC")) {
    debugLog(1, "[filter] skipped (LoC " + std::to_string(count) + " < minFileLoC " +
                    std::to_string(fileSettings.at("minFileLoC")) + "): " + fileName);
    return false;
  }
  if (count > fileSettings.at("maxFileLoC")) {
    debugLog(1, "[filter] skipped (LoC " + std::to_string(count) + " > maxFileLoC " +
                    std::to_string(fileSettings.at("maxFileLoC")) + "): " + fileName);
    return false;
  }
  return true;
}

int Filterer::getAllCFiles(std::filesystem::path pathObject,
                           std::vector<std::string> &filesToFilter, int numFiles) {
  if (!std::filesystem::exists(pathObject)) {
    debugLog(2, "[filter] path does not exist: " + pathObject.string());
    return 0;
  }
  if (std::filesystem::is_regular_file(pathObject)) {
    if (pathObject.has_extension()) {
      if (pathObject.extension() == ".c") {
        debugLog(2, "[filter] queued: " + pathObject.filename().string());
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

/// Main driver for the Filter System
int Filterer::run() {
  std::filesystem::path pathObject(configuration.databaseDir);
  std::vector<std::string> filesToFilter;

  debugLog(1, "[filter] scanning: " + pathObject.string());

  int filesFound = getAllCFiles(pathObject, filesToFilter, 0);
  debugLog(1, "[filter] found " + std::to_string(filesFound) + " .c file(s)");

  int passed = 0;
  for (const std::string &fileName : filesToFilter) {
    if (!checkPotentialFile(fileName))
      continue;
    passed++;

    std::filesystem::path oldPath(fileName);
    // Mirror the input's path under filterDir by stripping the databaseDir
    // prefix; relative() handles absolute and relative inputs uniformly.
    std::filesystem::path relPath = std::filesystem::relative(oldPath, configuration.databaseDir);
    if (relPath.empty() || *relPath.begin() == "..")
      relPath = oldPath.filename();
    std::filesystem::path newPath = std::filesystem::path(configuration.filterDir) / relPath;

    // Hard guard: never write over the source, whatever the path arithmetic.
    if (std::filesystem::weakly_canonical(oldPath) == std::filesystem::weakly_canonical(newPath)) {
      std::cerr << "Refusing to overwrite source file: " << oldPath.string() << std::endl;
      continue;
    }

    std::filesystem::create_directories(newPath.parent_path());
    std::error_code ec;
    llvm::raw_fd_ostream output(llvm::StringRef(newPath.string()), ec);

    FrontendFactoryWithArgs factory(&complexityConfig, &featureConfig, output);
    bool ran = runToolOnFile(oldPath.string(), factory);
    output.close();
    if (!ran)
      continue;

    if (globalDebugLevel() >= 2 && std::filesystem::exists(newPath)) {
      std::ifstream outFile(newPath.string());
      if (outFile.is_open()) {
        std::stringstream contents;
        contents << outFile.rdbuf();
        debugLog(2, "[filter] output for " + newPath.string() + ":\n" + contents.str());
      }
    }
  }

  std::cout << "\n=== Filter summary ===\n"
            << "  Files found:            " << filesFound << "\n"
            << "  Passed pre-filter:      " << passed << "\n"
            << "  Skipped:                " << (filesFound - passed) << std::endl;
  return 0;
}
