// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConfigParser.hpp"
#include "CountingVisitor.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief A single property entry in an SV-Comp .yml task file.
 *
 * Maps to one block under the {@code properties:} key. For now every benchmark
 * gets the same fixed set; later, {@code selectProperties} will choose based on
 * AST characteristics (loops → termination, integer arithmetic → no-overflow,
 * etc.).
 */
struct BenchmarkProperty {
  std::string propertyFile; ///< Relative path to the .prp file (e.g. "../properties/termination.prp").
  bool expectedVerdict;     ///< {@code true} = program satisfies the property.
};

/**
 * @brief Runtime configuration for the verify stage.
 */
struct verifyConfigs {
  int debugLevel;           ///< Verbosity level for debug output.
  bool keepCompilesOnly;    ///< If true, delete output files that fail checkCompilable.
  std::string transformDir; ///< Input directory of transformed C files to verify.
  std::string benchmarkDir; ///< Output directory for finalized benchmarks.
};

/**
 * @brief Top-level orchestrator for the verify step — the third stage of the
 * pipeline (filter → transform → verify).
 *
 * Reparses each transformed file from scratch, so the fresh AST reflects
 * what the transform actually produced (transform edits are text-only and
 * invisible to the AST they were derived from). On that AST it re-applies
 * the filter's per-function thresholds — a function can fall below them
 * when havocking drops its void calls or prunes emptied control flow — and
 * repairs failures by stripping the body and unharnessing the call in the
 * generated main. A benchmark whose harness empties out is discarded.
 *
 * Verify also owns benchmark finalization: the isolated compile check, the
 * .yml task file, and preprocessing to the .i the task file references.
 */
class Verifier {
public:
  /**
   * @brief Constructs a Verifier and immediately parses the config file.
   *
   * @param configFile Path to the INI-style properties file ("" = defaults only).
   * @param inputPath  Optional directory (or single .c file) of transformed
   *                   files to verify. When given, it overrides transformDir,
   *                   and benchmarkDir defaults to "<name>-benchmarks" (a
   *                   benchmarkDir set in the config still wins).
   */
  Verifier(std::string configFile, std::string inputPath = "");

  /**
   * @brief Verifies and finalizes a single transformed C file.
   *
   * Runs the VerifyAction re-check/repair pass, writing the (possibly
   * repaired) source to benchmarkDir, then discards empty-harness or
   * non-compiling results and emits the .yml + .i for survivors.
   *
   * @param path Path to the transformed C source file.
   * @return true if a finalized benchmark (.c + .yml + .i) was produced.
   */
  bool verifyFile(std::filesystem::path path);

  /**
   * @brief Recursively walks a directory tree, verifying every .c file found.
   *
   * @param path Root path to search (file or directory).
   * @return Total count of finalized benchmarks produced.
   */
  int verifyAll(std::filesystem::path path);

  /**
   * @brief Checks whether a verified file compiles without errors.
   *
   * Runs `clang -fsyntax-only` against the file alongside a dummy
   * `verifier.c` (to resolve extern `__VERIFIER_nondet_*` declarations).
   *
   * @param path Path to the C file to check.
   * @return true if the file compiles with no errors.
   */
  bool checkCompilable(std::filesystem::path path);

  /**
   * @brief Main entry point — runs verifyAll over transformDir.
   *
   * @return The number of finalized benchmarks produced.
   */
  int run();

  /**
   * @brief Returns the set of verification properties for a benchmark.
   *
   * Currently returns a fixed set (termination + no-overflow) for every file,
   * ignoring the counts. Future: use the per-function counts/features from
   * the verify pass to conditionally include properties (e.g. only
   * termination if loops are present, only no-overflow if integer arithmetic
   * is present, unreach-call when {@code reach_error()} guards exist).
   *
   * @param counts Per-function counts from the verify pass over the final source.
   * @return Vector of properties to include in the task .yml.
   */
  std::vector<BenchmarkProperty> selectProperties(
      const std::unordered_map<std::string, CountingVisitor::attributes> &counts);

  /**
   * @brief Writes an SV-Comp .yml task definition alongside the benchmark .c file.
   *
   * The task file references the preprocessed {@code .i} form of the input.
   * Properties are selected via {@code selectProperties()}.
   *
   * @param cPath  Path to the finalized .c benchmark file.
   * @param counts Per-function counts, forwarded to selectProperties.
   */
  void writeBenchmarkTask(std::filesystem::path cPath,
                          const std::unordered_map<std::string, CountingVisitor::attributes> &counts);

  /**
   * @brief Preprocesses a finalized .c file into a .i file.
   *
   * Runs {@code clang -E -P -std=gnu11} on the source file, writing the
   * preprocessed output alongside it with a {@code .i} extension.
   *
   * @param cPath Path to the finalized .c benchmark file.
   * @return true if preprocessing succeeded, false otherwise.
   */
  bool preprocess(std::filesystem::path cPath);

private:
  /// Thresholds and feature gates re-applied post-transform — the same
  /// PipelineConfig structure the filter stage applies pre-transform.
  PipelineConfig config;
  /// Path settings and flags for this stage.
  struct verifyConfigs configuration;
  /// Counts for the end-of-run summary.
  int _totalProcessed = 0;
  int _functionsUnharnessed = 0;
};
