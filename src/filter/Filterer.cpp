// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Filterer.hpp"
#include "FilterAction.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include "DebugLog.hpp"
#include "WorkerPool.hpp"

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
#include <unistd.h>

const std::string defaultDatabaseDir = "repos";
/// Per-file wall-clock budget for the isolated filter child, in seconds.
const int defaultFileTimeoutSecs = 60;

Filterer::Filterer(std::string configFile, std::string inputPath) {
  configuration.databaseDir = defaultDatabaseDir;
  configuration.fileTimeoutSecs = defaultFileTimeoutSecs;
  configuration.nproc = 0;
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

  if (!config.databaseDir.empty())
    configuration.databaseDir = config.databaseDir;
  if (!config.filterDir.empty())
    configuration.filterDir = config.filterDir;

  globalDebugLevel() = config.fileSettings.at("debugLevel");
  configuration.fileTimeoutSecs = config.fileSettings.at("fileTimeoutSecs");
  configuration.nproc = config.fileSettings.at("nproc");

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

// Mirror the input's path under filterDir by stripping the databaseDir
// prefix; relative() handles absolute and relative inputs uniformly.
// relative() yields "." when the input path IS the file (single-file mode).
std::filesystem::path Filterer::outputPath(std::filesystem::path oldPath) {
  std::filesystem::path relPath = std::filesystem::relative(oldPath, configuration.databaseDir);
  if (relPath.empty() || *relPath.begin() == ".." || relPath == ".")
    relPath = oldPath.filename();
  return std::filesystem::path(configuration.filterDir) / relPath;
}

bool Filterer::filterFile(std::filesystem::path oldPath) {
  std::filesystem::path newPath = outputPath(oldPath);

  // Hard guard: never write over the source, whatever the path arithmetic.
  if (std::filesystem::weakly_canonical(oldPath) == std::filesystem::weakly_canonical(newPath)) {
    debugLog(0, "Refusing to overwrite source file: " + oldPath.string());
    return false;
  }

  std::filesystem::create_directories(newPath.parent_path());
  std::error_code ec;
  llvm::raw_fd_ostream output(llvm::StringRef(newPath.string()), ec);
  if (ec) {
    debugLog(0, "Cannot open output file " + newPath.string() + ": " + ec.message());
    return false;
  }

  std::vector<std::string> includeDirs = collectLocalIncludeDirs(oldPath, *headerIndex);

  FrontendFactoryWithArgs factory(&config.complexity, &config.features, output);
  bool ran = runToolOnFile(oldPath.string(), factory, includeDirs);
  output.close();
  if (!ran) {
    debugLog(1, "[filter] clang tool failed on: " + oldPath.string());
    // The stream above already created/truncated newPath.
    cleanupPartialOutput(oldPath);
    return false;
  }
  return true;
}

void Filterer::cleanupPartialOutput(std::filesystem::path oldPath) {
  std::filesystem::path newPath = outputPath(oldPath);
  // The same guard filterFile applies before writing: when databaseDir and
  // filterDir resolve to one directory, the "partial output" is the input.
  if (std::filesystem::weakly_canonical(oldPath) == std::filesystem::weakly_canonical(newPath))
    return;
  std::error_code ec;
  std::filesystem::remove(newPath, ec);
}

int Filterer::run() {
  std::filesystem::path pathObject(configuration.databaseDir);
  std::vector<std::string> filesToFilter;

  debugLog(1, "[filter] scanning: " + pathObject.string());

  int filesFound = getAllCFiles(pathObject, filesToFilter, 0);
  debugLog(1, "[filter] found " + std::to_string(filesFound) + " .c file(s)");

  // Built once and reused for every file below, so resolving local
  // #includes to -I paths doesn't re-walk databaseDir per file.
  headerIndex.emplace(configuration.databaseDir);

  int passed = 0;
  std::vector<std::filesystem::path> toProcess;
  for (const std::string &fileName : filesToFilter) {
    debugLog(1, "[filter] file: " + fileName);
    if (!checkPotentialFile(fileName))
      continue;
    passed++;
    toProcess.emplace_back(fileName);
  }

  int workers = resolveWorkerCount(configuration.nproc);
  debugLog(1, "[filter] worker pool size: " + std::to_string(workers));

  IsolatedWork work;
  work.child = [this](const std::filesystem::path &p) {
    bool produced = filterFile(p);
    std::cout.flush();
    std::cerr.flush();
    _exit(produced ? kProducedExit : kDeclinedExit);
  };
  work.runInProcess = [this](const std::filesystem::path &p) { return filterFile(p); };
  work.cleanupPartial = [this](const std::filesystem::path &p) { cleanupPartialOutput(p); };
  work.debugLog = [](int level, const std::string &msg) { debugLog(level, "[filter] " + msg); };
  work.label = "filter";

  std::cout << "[filter] processing " << toProcess.size() << " file(s) with " << workers
            << " worker(s)" << std::endl;
  WorkerPoolResult result = runWorkerPool(toProcess, workers, configuration.fileTimeoutSecs, work);

  std::cout << "\n=== Filter summary ===\n"
            << "  Files found:            " << filesFound << "\n"
            << "  Passed pre-filter:      " << passed << "\n"
            << "  Skipped (pre-filter):   " << (filesFound - passed) << "\n"
            << "  Files filtered:         " << result.produced << "\n"
            << "  Declined (no output):   " << result.declined << "\n"
            << "  Failed:                 " << result.failed << std::endl;
  return result.produced;
}
