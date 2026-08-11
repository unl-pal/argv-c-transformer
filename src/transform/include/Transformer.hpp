// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "IncludeIndex.hpp"

#include <filesystem>
#include <optional>
#include <string>

/**
 * @brief Runtime configuration loaded from the INI-style config file.
 *
 * Holds the path settings and flags that control where filtered input is
 * read from and where transformed files are written.
 */
struct transformConfigs {
  int debugLevel;           ///< Verbosity level for debug output (see DebugLog.hpp).
  std::string filterDir;    ///< Input directory containing filtered C files to transform.
  std::string transformDir; ///< Output directory for transformed files (verify-stage input).
  int fileTimeoutSecs;      ///< Wall-clock budget per file for the isolated transform child.
  /// Original repo tree the filtered files came from, used only to resolve
  /// quoted #includes to -I paths. Empty means no local-header resolution.
  std::string databaseDir;
};

/**
 * @brief Top-level orchestrator for the transform step.
 *
 * Reads a config file, walks a directory tree of filtered C source files, and
 * runs the full Clang AST pipeline on each one: replacing dead calls with
 * `__VERIFIER_nondet_*`, injecting verifier declarations, and ensuring a
 * `main()` exists. Transformed output is written to transformDir under a
 * single flattened filename per input (see flattenedOutputPath).
 *
 * Transform is purely source→source; benchmark finalization (metric
 * re-check, compile check, .yml task files, preprocessing) happens in the
 * verify stage that follows.
 */
class Transformer {
public:
  /**
   * @brief Constructs a Transformer and immediately parses the config file.
   *
   * @param configFile Path to the INI-style properties file ("" = defaults only).
   * @param inputPath  Optional directory (or single .c file) of filtered files
   *                   to transform. Overrides filterDir and derives
   *                   transformDir as "<name>-transformed", both taking
   *                   precedence over the config file.
   */
  Transformer(std::string configFile, std::string inputPath = "");

  /**
   * @brief Runs the full Clang AST pipeline on a single C file.
   *
   * Builds a ClangTool invocation, runs the TransformAction consumer chain,
   * and writes the rewritten source to transformDir. A file whose generated
   * main harnesses nothing is discarded immediately (see harnessIsEmpty).
   *
   * @param path Path to the filtered C source file to transform.
   * @return true if a transformed .c was produced.
   */
  bool transformFile(std::filesystem::path path);

  /**
   * @brief Runs {@code transformFile} in a forked child for crash isolation.
   *
   * The child does the file I/O and reports success through
   * its exit status; the parent enforces {@code fileTimeoutSecs} and cleans up
   * any partial output left by a child that crashed or timed out.
   *
   * @param path Path to the filtered C source file to transform.
   * @return 1 if a transformed file was produced, 0 otherwise.
   */
  int transformFileIsolated(std::filesystem::path path);

  /**
   * @brief Computes the flattened transformDir output path for a filtered file.
   *
   * @param path Path to the filtered C source file.
   * @return The {@code transformDir/<flattened>.c} path the transform writes to.
   */
  std::filesystem::path flattenedOutputPath(std::filesystem::path path);

  /**
   * @brief Removes any .c a crashed or timed-out child left behind.
   *
   * @param path Path to the filtered C source file whose output to clean up.
   */
  void cleanupPartialOutput(std::filesystem::path path);

  /**
   * @brief Recursively walks a directory tree, transforming every .c file found.
   *
   * @param path Root path to search (file or directory).
   * @return Total count of transformed files produced.
   */
  int transformAll(std::filesystem::path path);

  /**
   * @brief Parses the config file via the shared {@code parsePipelineConfig},
   * keeping the transform-relevant settings.
   *
   * @param configFile Path to the INI-style properties file.
   */
  void parseConfig(std::string configFile);

  /**
   * @brief Main entry point - runs transformAll over filterDir.
   *
   * @return The number of transformed files produced.
   */
  int run();

  /**
   * @brief Returns the resolved output directory the transform writes to.
   *
   * Lets the full pipeline point the verify stage at the transform's actual
   * output, whichever of default / config / derived-from-input won.
   */
  const std::string &getTransformDir() const { return configuration.transformDir; }

  /**
   * @brief Returns the resolved input directory of filtered files to transform.
   *
   * Lets the driver check this exists before calling run().
   */
  const std::string &getFilterDir() const { return configuration.filterDir; }

  /**
   * @brief Points local-#include resolution at the original repo tree.
   *
   * filterDir only mirrors .c files, so a filtered file's quoted #includes
   * can only be resolved against the tree the filter read from. Must be
   * called before run(); the header index is built there.
   */
  void setDatabaseDir(const std::string &dir) { configuration.databaseDir = dir; }

private:
  /// Path settings and flags loaded from the config file.
  struct transformConfigs configuration;
  /// Count of .c files attempted across the run, for the end-of-run summary.
  int _totalProcessed = 0;
  /// Header basename → directory index over databaseDir, built once in run()
  /// and used to resolve each file's quoted #includes to -I search paths.
  std::optional<HeaderIndex> headerIndex;
};
