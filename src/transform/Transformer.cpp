// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
// SPDX-License-Identifier: Apache-2.0

#include "include/Transformer.hpp"
#include "TransformAction.hpp"
#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include "ConfigParser.hpp"
#include "DebugLog.hpp"
#include "IncludeIndex.hpp"
#include "WorkerPool.hpp"

#include <filesystem>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <system_error>
#include <vector>
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
  configuration.nproc = 0;
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
  debugLog(1, "[transform] file: " + path.string());
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
    // The stream above already created/truncated srcPath.
    cleanupPartialOutput(path);
    return false;
  }

  if (harnessIsEmpty(srcPath)) {
    debugLog(1, "[transform] discarded (harness empty, nothing havocked/harnessed): " +
                    srcPath.string());
    cleanupPartialOutput(path);
    return false;
  }
  return true;
}

void Transformer::cleanupPartialOutput(std::filesystem::path path) {
  std::filesystem::path outPath = flattenedOutputPath(path);
  // A top-level file flattens to its own name, so filterDir == transformDir
  // makes the "partial output" the input itself.
  if (std::filesystem::weakly_canonical(path) == std::filesystem::weakly_canonical(outPath))
    return;
  std::error_code ec;
  std::filesystem::remove(outPath, ec);
}

void Transformer::collectCFiles(std::filesystem::path path, std::vector<std::filesystem::path> &files) {
  if (!std::filesystem::exists(path)) {
    debugLog(1, "[transform] path does not exist: " + path.string());
    return;
  }
  if (std::filesystem::is_directory(path)) {
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(path)) {
      collectCFiles(entry.path(), files);
    }
    return;
  }
  if (std::filesystem::is_regular_file(path)) {
    if (path.extension() == ".c") {
      files.push_back(path);
    } else {
      debugLog(3, "[transform] skipped (not .c): " + path.filename().string());
    }
    return;
  }
  debugLog(3, "[transform] ignored: " + path.filename().string());
}

WorkerPoolResult Transformer::transformAll(std::filesystem::path path) {
  std::vector<std::filesystem::path> files;
  collectCFiles(path, files);
  _totalProcessed = static_cast<int>(files.size());

  int workers = resolveWorkerCount(configuration.nproc);
  debugLog(1, "[transform] worker pool size: " + std::to_string(workers));
  std::cout << "[transform] processing " << files.size() << " file(s) with " << workers
            << " worker(s)" << std::endl;

  IsolatedWork work;
  work.child = [this](const std::filesystem::path &p) {
    bool produced = transformFile(p);
    std::cout.flush();
    std::cerr.flush();
    _exit(produced ? kProducedExit : kDeclinedExit);
  };
  work.runInProcess = [this](const std::filesystem::path &p) { return transformFile(p); };
  work.cleanupPartial = [this](const std::filesystem::path &p) { cleanupPartialOutput(p); };
  work.debugLog = [](int level, const std::string &msg) { debugLog(level, "[transform] " + msg); };
  work.label = "transform";

  return runWorkerPool(files, workers, configuration.fileTimeoutSecs, work);
}

void Transformer::parseConfig(std::string configFile) {
  PipelineConfig config = parsePipelineConfig(configFile);
  configuration.debugLevel = config.fileSettings.at("debugLevel");
  configuration.fileTimeoutSecs = config.fileSettings.at("fileTimeoutSecs");
  configuration.nproc = config.fileSettings.at("nproc");
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

  WorkerPoolResult result = transformAll(path);
  std::cout << "\n=== Transform summary ===\n"
            << "  Files processed:        " << _totalProcessed << "\n"
            << "  Files transformed:      " << result.produced << "\n"
            << "  Declined (no output):   " << result.declined << "\n"
            << "  Failed:                 " << result.failed << "\n"
            << "  Time elapsed:           "
            << formatElapsed(std::chrono::steady_clock::now() - startTime) << std::endl;
  return result.produced;
}
