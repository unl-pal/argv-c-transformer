// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConfigParser.hpp"
#include "IncludeIndex.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Runtime configuration loaded from the INI-style config file.
 *
 * Holds the path settings and flags that live outside the per-function
 * config maps - i.e. things that are not per-function complexity ranges or
 * feature gates.
 */
struct filterConfigs {
  std::string databaseDir; ///< Directory containing the source repos to filter
  std::string filterDir;   ///< Output directory for files that pass the filter
  int fileTimeoutSecs;     ///< Wall-clock budget per file for the isolated filter child.
  int nproc;               ///< Worker pool size (0 = default to hardware concurrency).
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
   * @param configFile Path to the INI-style properties file ("" = defaults only).
   * @param inputPath  Optional directory or .c file to filter. When given, it
   *                   overrides databaseDir and sets filterDir to
   *                   "<name>-filtered", both taking precedence over
   *                   whatever the config file sets for those keys.
   */
  Filterer(std::string configFile, std::string inputPath = "");

  /**
   * @brief Parses the config file via the shared {@code parsePipelineConfig},
   * then applies the filter-specific path handling (existence checks,
   * filterDir creation) and debug-level plumbing.
   *
   * @param configFile Path to the INI-style properties file.
   */
  void parseConfigFile(std::string configFile);

  /**
   * @brief Quick pre-filter check run before the full Clang AST pass.
   *
   * Rejects files whose non-empty line count falls outside
   * [minFileLoC, maxFileLoC].
   *
   * @param fileName Path to the C source file to check.
   * @return {@code true} if the file should proceed to the AST pass.
   */
  bool checkPotentialFile(std::string fileName);

  /**
   * @brief Computes the mirrored filterDir output path for a source file.
   *
   * @param oldPath Path to the source .c file, under databaseDir.
   * @return The {@code filterDir/<relative path>} the filter writes to.
   */
  std::filesystem::path outputPath(std::filesystem::path oldPath);

  /**
   * @brief Runs the full Clang AST pipeline on a single C file.
   *
   * Writes the filtered result to the mirrored path under filterDir.
   *
   * @param oldPath Path to the source .c file to filter.
   * @return true if a filtered .c was produced.
   */
  bool filterFile(std::filesystem::path oldPath);

  /**
   * @brief Removes any .c a crashed or timed-out child left behind.
   *
   * @param oldPath Path to the source .c file whose output to clean up.
   */
  void cleanupPartialOutput(std::filesystem::path oldPath);

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
   * @brief Main entry point - collects files, pre-filters, and runs the AST pipeline.
   *
   * For each .c file that passes {@code checkPotentialFile}, builds a Clang
   * tool invocation and runs the full filter consumer chain (count → filter →
   * remove), writing the result to filterDir.
   *
   * @return 0 on success.
   */
  int run();

  /**
   * @brief Returns the resolved output directory the filter writes to.
   *
   * Lets the full pipeline point the transform step at the filter's actual
   * output, whichever of default / config / derived-from-input won.
   */
  const std::string &getFilterDir() const { return configuration.filterDir; }

  /**
   * @brief Returns the resolved input tree the filter read from.
   *
   * Lets the full pipeline point the transform step's local-#include
   * resolution back at the real repo tree (filterDir only mirrors .c files,
   * not headers), whichever of default / config / derived-from-input won.
   */
  const std::string &getDatabaseDir() const { return configuration.databaseDir; }

private:
  /**
   * Thresholds, feature gates, and file settings from the shared config
   * parser; the same structure the verify stage re-applies post-transform.
   */
  PipelineConfig config;

  /** Path settings and flags that don't fit either config map. */
  struct filterConfigs configuration;

  /**
   * Header basename → directory index over databaseDir, built once in run()
   * and used to resolve each file's quoted #includes to -I search paths.
   */
  std::optional<HeaderIndex> headerIndex;
};
