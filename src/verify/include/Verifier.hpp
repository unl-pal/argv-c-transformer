// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConfigParser.hpp"
#include "CountingVisitor.hpp"
#include "WorkerPool.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

/** @brief A single property entry in an SV-Comp .yml task file - one block under the {@code
 * properties:} key. */
struct BenchmarkProperty {
  std::string
      propertyFile;     ///< Relative path to the .prp file (e.g. "../properties/termination.prp").
  bool expectedVerdict; ///< {@code true} = program satisfies the property.
};

/**
 * @brief Runtime configuration for the verify stage.
 */
struct verifyConfigs {
  int debugLevel;           ///< Verbosity level for debug output.
  bool keepCompilesOnly;    ///< If true, delete output files that fail checkCompilable.
  std::string transformDir; ///< Input directory of transformed C files to verify.
  std::string benchmarkDir; ///< Output directory for finalized benchmarks.
  int fileTimeoutSecs;      ///< Wall-clock budget per file for the isolated verify child.
  int fileMemoryMB = 0;     ///< Per-child memory limit; 0 = auto.
  int nproc;                ///< Worker pool size (0 = auto, three quarters of detected cores).
  bool cleanOutput;         ///< If true, wipe a pre-populated benchmarkDir instead of erroring.
};

/**
 * @brief Top-level orchestrator for the verify step - the third stage of the
 * pipeline (filter → transform → verify).
 *
 * Reparses each transformed file and re-applies the filter's thresholds,
 * stripping a rejected function's body and dropping its harness block. Each
 * remaining block becomes its own benchmark; a file with none left is discarded.
 *
 * Verify also owns benchmark finalization: the compile check (once per input
 * file), and per benchmark the .yml task file and preprocessing to the .i the
 * task file references.
 */
class Verifier {
public:
  /**
   * @brief Constructs a Verifier and immediately parses the config file.
   *
   * @param configFile Path to the INI-style properties file ("" = defaults only).
   * @param inputPath  Optional directory (or single .c file) of transformed
   *                   files to verify. Overrides transformDir and derives
   *                   benchmarkDir as "<name>-benchmarks" (unless input is
   *                   named the default transformDir), both taking
   *                   precedence over the config file.
   */
  Verifier(std::string configFile, std::string inputPath = "");

  /**
   * @brief Verifies a single transformed C file and splits it into benchmarks.
   *
   * Runs the VerifyAction re-check/repair pass and compile-checks the
   * combined result once. If it compiles, writes one benchmark per surviving
   * harness block, {@code <stem>__<function>.c} (see splitPath), each with
   * its own .yml + .i.
   *
   * @param path Path to the transformed C source file.
   * @return true if at least one finalized benchmark (.c + .yml + .i) was
   *         produced. A false return leaves nothing behind except under
   *         keepCompilesOnly=false, which keeps the non-compiling combined
   *         {@code <stem>.c} on purpose.
   */
  bool verifyFile(std::filesystem::path path);

  /**
   * @brief Derives one harnessed function's benchmark path from the combined file's path.
   *
   * @return {@code <basePath's dir>/<stem>__<target><ext>}.
   */
  static std::filesystem::path splitPath(const std::filesystem::path &basePath,
                                         const std::string &target);

  /**
   * @brief Removes every output of one input file: the combined .c and each
   * split benchmark's .c/.yml/.i. Never removes the input itself.
   *
   * @param path Path to the transformed C source file whose outputs to clean up.
   */
  void cleanupPartialOutput(std::filesystem::path path);

  /**
   * @brief Recursively walks a directory tree, verifying every .c file found.
   *
   * Collects the matching files, then runs them through a worker pool sized by
   * {@code configuration.nproc}, each file isolated in its own forked child.
   *
   * @param path Root path to search (file or directory).
   * @return Per-outcome counts across the tree.
   */
  WorkerPoolResult verifyAll(std::filesystem::path path);

  /**
   * @brief Recursively collects every .c file under path into `files`.
   *
   * @param path  Root path to search (file or directory).
   * @param files Output vector; matching file paths are appended.
   */
  void collectCFiles(std::filesystem::path path, std::vector<std::filesystem::path> &files);

  /**
   * @brief Checks whether a verified file compiles without errors.
   *
   * Runs clang::SyntaxOnlyAction in-process via libTooling (the same class
   * `clang -fsyntax-only` selects). No stub definitions are needed for
   * `__VERIFIER_nondet_*`: syntax-only checking never links, and
   * argv_c_harness.h (which every benchmark unconditionally `#include`s)
   * already supplies the extern declarations.
   *
   * @param path Path to the C file to check.
   * @return true if the file compiles with no errors.
   */
  bool checkCompilable(std::filesystem::path path);

  /**
   * @brief Main entry point - runs verifyAll over transformDir.
   *
   * @return The number of finalized benchmarks produced.
   */
  int run();

  /**
   * @brief Returns the resolved input directory of transformed files to verify.
   *
   * Lets the driver check this exists before calling run().
   */
  const std::string &getTransformDir() const { return configuration.transformDir; }

  /**
   * @brief Returns the resolved output directory verify writes benchmarks to.
   *
   * Lets the full pipeline preflight-check it before any stage runs.
   */
  const std::string &getBenchmarkDir() const { return configuration.benchmarkDir; }

  /**
   * @brief Returns the set of verification properties for a benchmark.
   *
   * Currently a fixed set (termination + no-overflow) for every file, ignoring
   * counts; the hook for future AST-driven property selection.
   *
   * @param counts Per-function counts from the verify pass over the final source.
   * @return Vector of properties to include in the task .yml.
   */
  std::vector<BenchmarkProperty>
  selectProperties(const std::unordered_map<std::string, CountingVisitor::attributes> &counts);

  /**
   * @brief Writes an SV-Comp .yml task definition alongside the benchmark .c file.
   *
   * The task file references the preprocessed {@code .i} form of the input.
   * Properties are selected via {@code selectProperties()}.
   *
   * @param cPath  Path to the finalized .c benchmark file.
   * @param counts Per-function counts, forwarded to selectProperties.
   */
  void
  writeBenchmarkTask(std::filesystem::path cPath,
                     const std::unordered_map<std::string, CountingVisitor::attributes> &counts);

  /**
   * @brief Preprocesses a finalized .c file into a .i file.
   *
   * Runs clang::PrintPreprocessedAction in-process via libTooling (the same
   * class {@code clang -E -P -std=gnu11} selects), writing the preprocessed
   * output alongside the source with a {@code .i} extension.
   *
   * @param cPath Path to the finalized .c benchmark file.
   * @return true if preprocessing succeeded, false otherwise.
   */
  bool preprocess(std::filesystem::path cPath);

private:
  /**
   * @brief Writes the embedded argv_c_harness.h into benchmarkDir.
   */
  void writeHarnessHeader();

  /** @brief Writes text to path, logging on failure. @return false if the write failed. */
  static bool writeText(const std::filesystem::path &path, const std::string &text);

  /** @brief The subset of counts that describes one benchmark: its target plus file-wide markers. */
  static std::unordered_map<std::string, CountingVisitor::attributes>
  targetCounts(const std::unordered_map<std::string, CountingVisitor::attributes> &counts,
               const std::string &target);

  /** @brief Removes a benchmark's .c, .yml and .i. */
  static void removeBenchmark(const std::filesystem::path &cPath);

  /**
   * Thresholds and feature gates re-applied post-transform - the same
   * PipelineConfig structure the filter stage applies pre-transform.
   */
  PipelineConfig config;
  /** Path settings and flags for this stage. */
  struct verifyConfigs configuration;
  /** Count for the end-of-run summary. */
  int _totalProcessed = 0;
};
