// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CountingVisitor.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Runtime configuration loaded from the INI-style config file.
 *
 * Holds the path settings and flags that live outside the per-function
 * config maps — i.e. things that are not per-function complexity ranges or
 * feature gates.
 */
struct filterConfigs {
  std::string databaseDir; ///< Directory containing the source repos to filter
  std::string filterDir;   ///< Output directory for files that pass the filter
  bool wipeOldBenchmarks;  ///< Not yet implemented; reserved for future use
};

/**
 * @brief Top-level orchestrator for the filter step.
 *
 * Reads a config file, walks a directory tree of C source files, applies a
 * quick pre-filter (header check + line-count bounds), then runs the full
 * Clang AST pipeline on each file that passes. Filtered output is written to
 * filterDir, mirroring the original directory structure.
 */
class Filterer {
public:
  /**
   * @brief Constructs a Filterer and immediately parses the config file.
   *
   * @param configFile Path to the INI-style properties file.
   */
  Filterer(std::string configFile);

  /**
   * @brief Parses the config file, populating the two per-function config
   * maps and the file-level settings.
   *
   * Complexity metric keys (e.g. {@code ForLoops}) are parsed as a
   * {@code min,max} pair into {@code complexityConfig}. Feature keys (e.g.
   * {@code Concurrency}) are parsed as {@code ignore|require|forbid} into
   * {@code featureConfig}. File-level settings (e.g. {@code minFileLoC},
   * {@code useNonStdHeaders}) go into {@code fileSettings}. Path settings go
   * into {@code configuration}. Unknown keys are reported to stderr.
   *
   * @param configFile Path to the INI-style properties file.
   */
  void parseConfigFile(std::string configFile);

  /**
   * @brief Quick pre-filter check run before the full Clang AST pass.
   *
   * Rejects files that include non-standard headers (when useNonStdHeaders is
   * 0) or whose non-empty line count falls outside [minFileLoC, maxFileLoC].
   *
   * @param fileName Path to the C source file to check.
   * @return {@code true} if the file should proceed to the AST pass.
   */
  bool checkPotentialFile(std::string fileName);

  /**
   * @brief Recursively collects all .c files under a path into a vector.
   *
   * Accepts either a single file or a directory. Directories are walked
   * recursively. Non-.c files and files without extensions are skipped.
   *
   * @param pathObject    Root path to search.
   * @param filesToFilter Output vector; matching file paths are appended.
   * @param numFiles      Running count of files found (default 0).
   * @return Total number of .c files found.
   */
  int getAllCFiles(std::filesystem::path pathObject, std::vector<std::string> &filesToFilter,
                   int numFiles = 0);

  /**
   * @brief Main entry point — collects files, pre-filters, and runs the AST pipeline.
   *
   * For each .c file that passes {@code checkPotentialFile}, builds a Clang
   * tool invocation and runs the full filter consumer chain (count → filter →
   * remove), writing the result to filterDir.
   *
   * @return 0 on success.
   */
  int run();

private:
  /// C standard library header names used to distinguish std from non-std includes.
  const std::vector<std::string> stdLibNames = {
      "assert.h",      "complex.h",  "ctype.h",  "errno.h",     "fenv.h",   "float.h",
      "inttypes.h",    "iso646.h",   "limits.h", "locale.h",    "math.h",   "setjmp.h",
      "signal.h",      "stdalign.h", "stdarg.h", "stdatomic.h", "stdbit.h", "stdbool.h",
      "stdckdint.h",   "stddef.h",   "stdint.h", "stdio.h",     "stdlib.h", "stdmchar.h",
      "stdnoreturn.h", "string.h",   "tgmath.h", "threads.h",   "time.h",   "uchar.h",
      "wchar.h",       "wctype.h",   "string"};

  /**
   * @brief Per-function complexity thresholds loaded from the config.
   *
   * Keys are complexity metric names (e.g. {@code ForLoops}); values are
   * {@code [min, max]} pairs. Defaults are {@code {0, 99999}}, meaning no
   * filtering unless explicitly configured.
   */
  std::map<std::string, std::pair<int, int>> complexityConfig = {
      {"CallFunc", {0, 99999}}, {"ForLoops", {0, 99999}},  {"Functions", {0, 99999}},
      {"IfStmt", {0, 99999}},   {"Param", {0, 99999}},     {"WhileLoops", {0, 99999}},
  };

  /**
   * @brief Per-function feature gates loaded from the config.
   *
   * Keys are feature names (e.g. {@code Concurrency}); values say whether a
   * function must have the feature present, must not have it, or the
   * feature is ignored for filtering. Defaults to {@code Ignore}.
   */
  std::map<std::string, FeatureGate> featureConfig = {
      {"Concurrency", FeatureGate::Ignore},
      {"FloatingPoint", FeatureGate::Ignore},
  };

  /**
   * @brief File-level settings that aren't per-function (e.g. LoC bounds).
   */
  std::map<std::string, int> fileSettings = {
      {"debugLevel", 0}, {"maxFileLoC", 99999}, {"minFileLoC", 0}, {"useNonStdHeaders", 0}};

  /// Path settings and flags that don't fit either config map.
  struct filterConfigs configuration;
};
