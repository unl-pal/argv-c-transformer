// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
// SPDX-License-Identifier: Apache-2.0

#include "include/Transformer.hpp"
#include "TransformAction.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include "ConfigParser.hpp"
#include "DebugLog.hpp"

#include <csignal>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <system_error>
#include <sys/wait.h>
#include <unistd.h>

const int defaultDebugLevel = 0;
const std::string defaultFilterDir = "filteredFiles";
const std::string defaultTransformDir = "transformedFiles";
/// Per-file wall-clock budget for the isolated transform child, in seconds.
const int defaultFileTimeoutSecs = 60;

Transformer::Transformer(std::string configFile, std::string inputPath) : configuration() {
  configuration.debugLevel = defaultDebugLevel;
  configuration.filterDir = defaultFilterDir;
  configuration.transformDir = defaultTransformDir;
  configuration.fileTimeoutSecs = defaultFileTimeoutSecs;
  if (!configFile.empty())
    parseConfig(configFile);
  if (!inputPath.empty()) {
    configuration.filterDir = inputPath;
    configuration.transformDir = inputBaseName(inputPath) + "-transformed";
  }
  globalDebugLevel() = configuration.debugLevel;
}

// Flatten the filtered path into a single filename under transformDir:
//   filtered-files/antirez/redis/src/endianconv.c
//   → transformed-files/antirez_redis_src_endianconv.c
// Strip the filterDir prefix (works for both relative and absolute paths),
// then join remaining components with underscores.
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
  debugLog(1, "[transform] file: " + path.string());
  if (!std::filesystem::exists(path))
    return false;

  std::filesystem::path srcPath = flattenedOutputPath(path);

  // Run TransformAction, writing the rewritten source to srcPath
  std::error_code ec;
  std::filesystem::create_directories(srcPath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(srcPath.string()), ec);
  if (ec) {
    std::cerr << "Cannot open output file " << srcPath.string() << ": " << ec.message()
              << std::endl;
    return false;
  }

  ArgsFrontendFactory factory(output);
  bool ran = runToolOnFile(path.string(), factory);
  output.close();
  if (!ran)
    return false;

  // harness may be empty due to unsupported transforming
  if (harnessIsEmpty(srcPath)) {
    std::filesystem::remove(srcPath);
    return false;
  }
  return true;
}

// Runs transformFile in a forked child so a crash, OOM-kill, assertion, or
// hang on a single pathological file cannot take down the whole batch. The
// child does the file I/O (the rewritten .c) and exits with 1 on a produced
// file, 0 otherwise; the parent enforces a wall-clock timeout and reaps the
// child, translating its fate into the success count.
int Transformer::transformFileIsolated(std::filesystem::path path) {
  _totalProcessed++;
  std::cout << "\r[transform] " << _totalProcessed << " processed" << std::flush;
  pid_t pid = fork();
  if (pid < 0) {
    std::cerr << "fork failed, transforming in-process: " << path.string() << std::endl;
    return transformFile(path) ? 1 : 0;
  }

  if (pid == 0) {
    // Child: do the work and report 1/0 through the exit status. _exit skips
    // C++ stream flushing, so flush explicitly first (stdout may be fully
    // buffered when redirected, e.g. under benchexec).
    int produced = transformFile(path) ? 1 : 0;
    std::cout.flush();
    std::cerr.flush();
    _exit(produced);
  }

  // Parent: poll for completion, killing the child if it overruns the budget.
  time_t deadline = time(nullptr) + configuration.fileTimeoutSecs;
  int status = 0;
  while (true) {
    pid_t done = waitpid(pid, &status, WNOHANG);
    if (done == pid)
      break;
    if (done < 0) {
      std::cerr << "waitpid failed for " << path.string() << std::endl;
      return 0;
    }
    if (time(nullptr) >= deadline) {
      std::cerr << "Timeout, killing transform of: " << path.string() << std::endl;
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
  // WIFSIGNALED: segfault, OOM-kill, etc. The child may have left a partial
  // .c behind; harnessIsEmpty never ran, so clean up.
  std::cerr << "Transform crashed (signal " << WTERMSIG(status) << "), skipping: "
            << path.string() << std::endl;
  cleanupPartialOutput(path);
  return 0;
}

// Remove any .c a crashed or timed-out child left half-written, so
// downstream steps never see a partial file.
void Transformer::cleanupPartialOutput(std::filesystem::path path) {
  std::error_code ec;
  std::filesystem::remove(flattenedOutputPath(path), ec);
}

int Transformer::transformAll(std::filesystem::path path) {
  if (!std::filesystem::exists(path))
    return 0;
  if (std::filesystem::is_directory(path)) {
    int successes = 0;
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(path)) {
      successes += transformAll(entry.path());
    }
    return successes;
  }
  if (std::filesystem::is_regular_file(path) && path.extension() == ".c")
    return transformFileIsolated(path);
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
}

int Transformer::run() {
  std::filesystem::path path(configuration.filterDir);
  if (!std::filesystem::exists(path)) {
    std::cerr << "Filter directory not found: " << configuration.filterDir << std::endl;
    return 0;
  }
  int result = transformAll(path);
  int discarded = _totalProcessed - result;
  std::cout << "\n=== Transform summary ===\n"
            << "  Files processed:        " << _totalProcessed << "\n"
            << "  Files transformed:      " << result << "\n"
            << "  Discarded/failed:       " << discarded << std::endl;
  return result;
}
