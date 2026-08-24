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
#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

const std::string defaultDatabaseDir = "repos";
/// Per-file wall-clock budget for the isolated filter child, in seconds.
const int defaultFileTimeoutSecs = 60;

Filterer::Filterer(std::string configFile, std::string inputPath) {
  configuration.databaseDir = defaultDatabaseDir;
  configuration.fileTimeoutSecs = defaultFileTimeoutSecs;
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
    return false;
  }
  return true;
}

int Filterer::filterFileIsolated(std::filesystem::path oldPath) {
  pid_t pid = fork();
  if (pid < 0) {
    debugLog(0, "fork failed, filtering in-process: " + oldPath.string());
    return filterFile(oldPath) ? 1 : 0;
  }

  if (pid == 0) {
    int produced = filterFile(oldPath) ? 1 : 0;
    std::cout.flush();
    std::cerr.flush();
    _exit(produced);
  }

  time_t deadline = time(nullptr) + configuration.fileTimeoutSecs;
  int status = 0;
  while (true) {
    pid_t done = waitpid(pid, &status, WNOHANG);
    if (done == pid)
      break;
    if (done < 0 && errno != EINTR) {
      debugLog(0, "waitpid failed for " + oldPath.string() + ", killing: " + strerror(errno));
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      cleanupPartialOutput(oldPath);
      return 0;
    }
    if (time(nullptr) >= deadline) {
      debugLog(0, "Timeout, killing filter of: " + oldPath.string());
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      cleanupPartialOutput(oldPath);
      return 0;
    }
    struct timespec nap = {0, 20 * 1000 * 1000}; // 20ms
    nanosleep(&nap, nullptr);
  }

  if (WIFEXITED(status))
    return WEXITSTATUS(status) == 1 ? 1 : 0;
  debugLog(0, "Filter crashed (signal " + std::to_string(WTERMSIG(status)) + "), skipping: " +
                  oldPath.string());
  cleanupPartialOutput(oldPath);
  return 0;
}

void Filterer::cleanupPartialOutput(std::filesystem::path oldPath) {
  std::error_code ec;
  std::filesystem::remove(outputPath(oldPath), ec);
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
  int produced = 0;
  int i = 0;
  for (const std::string &fileName : filesToFilter) {
    std::cout << "\r[filter] " << ++i << "/" << filesFound << std::flush;
    debugLog(1, "[filter] file: " + fileName);
    if (!checkPotentialFile(fileName))
      continue;
    passed++;

    std::filesystem::path oldPath(fileName);
    produced += filterFileIsolated(oldPath);
  }
  if (filesFound > 0)
    std::cout << std::endl;

  std::cout << "\n=== Filter summary ===\n"
            << "  Files found:            " << filesFound << "\n"
            << "  Passed pre-filter:      " << passed << "\n"
            << "  Skipped (pre-filter):   " << (filesFound - passed) << "\n"
            << "  Files filtered:         " << produced << "\n"
            << "  Discarded/failed:       " << (passed - produced) << std::endl;
  return produced;
}
