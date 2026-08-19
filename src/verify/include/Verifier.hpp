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

/** @brief A single property entry in an SV-Comp .yml task file - one block under the {@code properties:} key. */
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
  int fileTimeoutSecs;      ///< Wall-clock budget per file for the isolated verify child.
};

/**
 * @brief Top-level orchestrator for the verify step - the third stage of the
 * pipeline (filter → transform → verify).
 *
 * Reparses each transformed file, re-applies the filter's thresholds
 * by stripping the body and unharnessing the call in the generated main.
 * A benchmark whose harness empties out is discarded.
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
   *                   files to verify. Overrides transformDir and derives
   *                   benchmarkDir as "<name>-benchmarks" (unless input is
   *                   named the default transformDir), both taking
   *                   precedence over the config file.
   */
  Verifier(std::string configFile, std::string inputPath = "");

  /**
   * @brief Verifies and finalizes a single transformed C file.
   *
   * Runs the VerifyAction re-check/repair pass, writing the
   * source to benchmarkDir, then discards empty-harness or
   * non-compiling results and emits the .yml + .i for survivors.
   *
   * @param path Path to the transformed C source file.
   * @return true if a finalized benchmark (.c + .yml + .i) was produced.
   */
  bool verifyFile(std::filesystem::path path);

  /**
   * @brief Runs {@code verifyFile} in a forked child for crash isolation.
   *
   * @param path Path to the transformed C source file.
   * @return 1 if a finalized benchmark was produced, 0 otherwise.
   */
  int verifyFileIsolated(std::filesystem::path path);

  /**
   * @brief Removes any benchmark files a crashed or timed-out child left behind.
   *
   * @param path Path to the transformed C source file whose output to clean up.
   */
  void cleanupPartialOutput(std::filesystem::path path);

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
   * Runs `clang -fsyntax-only` against the file. No stub definitions are
   * needed for `__VERIFIER_nondet_*`: syntax-only checking never links, and
   * argv_c_runtime.h (which every benchmark unconditionally `#include`s)
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
   * @brief Returns the set of verification properties for a benchmark.
   *
   * Currently a fixed set (termination + no-overflow) for every file, ignoring
   * counts; the hook for future AST-driven property selection.
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
  /**
   * @brief Writes the embedded argv_c_runtime.h into benchmarkDir.
   */
  void writeRuntimeHeader();

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
