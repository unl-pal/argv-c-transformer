// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
// SPDX-License-Identifier: Apache-2.0

#include "include/Transformer.hpp"
#include "TransformAction.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include "ConfigParser.hpp"
#include "DebugLog.hpp"
#include "IncludeIndex.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <system_error>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

const int defaultDebugLevel = 0;
// Must match Filterer's fallback default, so a bare transform run lines up
// with a bare filter run.
const std::string defaultFilterDir = "repos-filtered";
const int defaultFileTimeoutSecs = 60;

Transformer::Transformer(std::string configFile, std::string inputPath) : configuration() {
  configuration.debugLevel = defaultDebugLevel;
  configuration.filterDir = defaultFilterDir;
  configuration.fileTimeoutSecs = defaultFileTimeoutSecs;
  if (!configFile.empty())
    parseConfig(configFile);
  if (configuration.transformDir.empty())
    configuration.transformDir = inputBaseName(configuration.filterDir) + "-transformed";
  if (!inputPath.empty()) {
    configuration.filterDir = inputPath;
    configuration.transformDir = inputBaseName(inputPath) + "-transformed";
  }
  globalDebugLevel() = configuration.debugLevel;
}

//   filtered-files/antirez/redis/src/endianconv.c
//   -> transformed-files/antirez_redis_src_endianconv.c
std::filesystem::path Transformer::flattenedOutputPath(std::filesystem::path path) {
  std::filesystem::path relPath = std::filesystem::relative(path, configuration.filterDir);
  // relative() yields "." when the input path IS the file (single-file mode).
  if (relPath.empty() || *relPath.begin() == ".." || relPath == ".")
    relPath = path.filename();
  std::string flatName;
  for (const std::filesystem::path &component : relPath) {
    std::string part = component.string();
    if (part == ".." || part == ".")
      continue;
    if (!flatName.empty())
      flatName += "_";
    flatName += part;
  }
  return std::filesystem::path(configuration.transformDir) / flatName;
}

bool Transformer::transformFile(std::filesystem::path path) {
  debugLog(1, "[transform] file " + std::to_string(_totalProcessed) + ": " + path.string());
  if (!std::filesystem::exists(path)) {
    debugLog(1, "[transform] path does not exist: " + path.string());
    return false;
  }

  std::filesystem::path srcPath = flattenedOutputPath(path);

  std::error_code ec;
  std::filesystem::create_directories(srcPath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(srcPath.string()), ec);
  if (ec) {
    debugLog(0, "Cannot open output file " + srcPath.string() + ": " + ec.message());
    return false;
  }

  // Quoted #includes resolve against the source repo, not the filtered tree,
  // which mirrors only .c files. An unset databaseDir leaves this empty.
  std::vector<std::string> includeDirs =
      headerIndex ? collectLocalIncludeDirs(path, *headerIndex) : std::vector<std::string>{};

  ArgsFrontendFactory factory(output, configuration.havoc);
  bool ran = runToolOnFile(path.string(), factory, includeDirs);
  output.close();
  if (!ran) {
    debugLog(1, "[transform] clang tool failed on: " + path.string());
    return false;
  }

  if (harnessIsEmpty(srcPath)) {
    debugLog(1, "[transform] discarded (harness empty, nothing havocked/harnessed): " +
                    srcPath.string());
    std::filesystem::remove(srcPath);
    return false;
  }
  return true;
}

int Transformer::transformFileIsolated(std::filesystem::path path) {
  _totalProcessed++;
  if (globalDebugLevel() == 0)
    std::cout << "\r[transform] " << _totalProcessed << " processed" << std::flush;
  pid_t pid = fork();
  if (pid < 0) {
    debugLog(0, "fork failed, transforming in-process: " + path.string());
    return transformFile(path) ? 1 : 0;
  }

  if (pid == 0) {
    // Child: _exit skips C++ stream flushing, so flush explicitly first.
    int produced = transformFile(path) ? 1 : 0;
    std::cout.flush();
    std::cerr.flush();
    _exit(produced);
  }

  // Parent: poll, killing the child if it overruns the budget.
  time_t deadline = time(nullptr) + configuration.fileTimeoutSecs;
  int status = 0;
  while (true) {
    pid_t done = waitpid(pid, &status, WNOHANG);
    if (done == pid)
      break;
    if (done < 0 && errno != EINTR) {
      debugLog(0, "waitpid failed for " + path.string() + ", killing: " + strerror(errno));
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      cleanupPartialOutput(path);
      return 0;
    }
    if (time(nullptr) >= deadline) {
      debugLog(0, "Timeout, killing transform of: " + path.string());
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      cleanupPartialOutput(path);
      return 0;
    }
    struct timespec nap = {0, 20 * 1000 * 1000}; // 20ms
    nanosleep(&nap, nullptr);
  }

  if (WIFEXITED(status))
    return WEXITSTATUS(status) == 1 ? 1 : 0;
  // WIFSIGNALED: harnessIsEmpty never ran, so a partial .c may be left behind.
  debugLog(0, "Transform crashed (signal " + std::to_string(WTERMSIG(status)) + "), skipping: " +
                  path.string());
  cleanupPartialOutput(path);
  return 0;
}

void Transformer::cleanupPartialOutput(std::filesystem::path path) {
  std::error_code ec;
  std::filesystem::remove(flattenedOutputPath(path), ec);
}

int Transformer::transformAll(std::filesystem::path path) {
  if (!std::filesystem::exists(path)) {
    debugLog(1, "[transform] path does not exist: " + path.string());
    return 0;
  }
  if (std::filesystem::is_directory(path)) {
    int successes = 0;
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(path)) {
      successes += transformAll(entry.path());
    }
    return successes;
  }
  if (std::filesystem::is_regular_file(path)) {
    if (path.extension() == ".c")
      return transformFileIsolated(path);
    debugLog(4, "[transform] skipped (not .c): " + path.filename().string());
    return 0;
  }
  debugLog(3, "[transform] ignored: " + path.filename().string());
  return 0;
}

void Transformer::parseConfig(std::string configFile) {
  PipelineConfig config = parsePipelineConfig(configFile);
  configuration.debugLevel = config.fileSettings.at("debugLevel");
  configuration.fileTimeoutSecs = config.fileSettings.at("fileTimeoutSecs");
  if (!config.transformDir.empty()) {
    configuration.transformDir = config.transformDir;
  }
  if (!config.filterDir.empty()) {
    configuration.filterDir = config.filterDir;
  }
  // argv-c injects this via setDatabaseDir(); a standalone `transform` run
  // only has the config.
  if (!config.databaseDir.empty())
    configuration.databaseDir = config.databaseDir;
  configuration.havoc.argcMin = config.havoc.at("havocArgcMin");
  configuration.havoc.argcMax = config.havoc.at("havocArgcMax");
  configuration.havoc.strMax = config.havoc.at("havocStrMax");
  configuration.havoc.blockMax = config.havoc.at("havocBlockMax");
  configuration.havoc.arrayElems = config.havoc.at("havocArrayElems");
}

int Transformer::run() {
  auto startTime = std::chrono::steady_clock::now();
  std::filesystem::path path(configuration.filterDir);
  // Built before any fork; each child inherits the index as-is.
  headerIndex.emplace(configuration.databaseDir);

  int result = transformAll(path);
  int discarded = _totalProcessed - result;
  std::cout << "\n=== Transform summary ===\n"
            << "  Files processed:        " << _totalProcessed << "\n"
            << "  Files transformed:      " << result << "\n"
            << "  Discarded/failed:       " << discarded << "\n"
            << "  Time elapsed:           "
            << formatElapsed(std::chrono::steady_clock::now() - startTime) << std::endl;
  return result;
}
