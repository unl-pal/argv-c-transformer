// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>
#include <string>
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
 * @brief Runtime configuration loaded from the INI-style config file.
 *
 * Holds the path settings and flags that control where filtered input is
 * read from, where benchmarks are written, and how compile failures are
 * handled.
 */
struct transformConfigs {
  int debugLevel;           ///< Verbosity level for debug output (currently unused).
  bool keepCompilesOnly;    ///< If true, delete output files that fail checkCompilable.
  std::string filterDir;    ///< Input directory containing filtered C files to transform.
  std::string benchmarkDir; ///< Output directory for transformed benchmark files.
  bool wipeOldBenchmarks;   ///< Not yet implemented; reserved for future use.
  int fileTimeoutSecs;      ///< Wall-clock budget per file for the isolated transform child.
};

/**
 * @brief Top-level orchestrator for the transform step.
 *
 * Reads a config file, walks a directory tree of filtered C source files, and
 * runs the full Clang AST pipeline on each one: replacing dead calls with
 * `__VERIFIER_nondet_*`, injecting verifier declarations, and ensuring a
 * `main()` exists. Transformed output is written to benchmarkDir, mirroring
 * the original directory structure.
 */
class Transformer {
public:
  /**
   * @brief Constructs a Transformer and immediately parses the config file.
   *
   * @param configFile Path to the INI-style properties file.
   */
  Transformer(std::string configFile);

  /**
   * @brief Runs the full Clang AST pipeline on a single C file.
   *
   * Builds a ClangTool invocation, runs the TransformAction consumer chain,
   * and writes the rewritten source to benchmarkDir. If the result fails
   * checkCompilable and keepCompilesOnly is set, the output file is removed.
   *
   * @param path Path to the filtered C source file to transform.
   * @return false if the AST tool failed to run; otherwise true.
   */
  bool transformFile(std::filesystem::path path);

  /**
   * @brief Runs {@code transformFile} in a forked child for crash isolation.
   *
   * A segfault, OOM-kill, assertion, or hang on a single pathological file
   * would otherwise take down the whole batch, since each file's ClangTool
   * runs in-process. The child does the file I/O and reports success through
   * its exit status; the parent enforces {@code fileTimeoutSecs} and cleans up
   * any partial output left by a child that crashed or timed out.
   *
   * @param path Path to the filtered C source file to transform.
   * @return 1 if a benchmark was produced, 0 otherwise.
   */
  int transformFileIsolated(std::filesystem::path path);

  /**
   * @brief Computes the flattened benchmarkDir output path for a filtered file.
   *
   * @param path Path to the filtered C source file.
   * @return The {@code benchmarkDir/<flattened>.c} path the transform writes to.
   */
  std::filesystem::path flattenedOutputPath(std::filesystem::path path);

  /**
   * @brief Removes any .c/.yml/.i a crashed or timed-out child left behind.
   *
   * @param path Path to the filtered C source file whose output to clean up.
   */
  void cleanupPartialOutput(std::filesystem::path path);

  /**
   * @brief Recursively walks a directory tree, transforming every .c file found.
   *
   * @param path  Root path to search (file or directory).
   * @param count Running count of compilable benchmarks produced so far.
   * @return Total count of compilable benchmarks produced.
   */
  int transformAll(std::filesystem::path path, int count);

  /**
   * @brief Checks whether a transformed file compiles without errors.
   *
   * Runs `clang -fsyntax-only` against the file alongside a dummy
   * `verifier.c` (to resolve extern `__VERIFIER_nondet_*` declarations).
   *
   * @param path Path to the transformed C file to check.
   * @return 1 if the file compiles with no errors, 0 otherwise.
   */
  int checkCompilable(std::filesystem::path path);

  /**
   * @brief Detects a trivial benchmark whose generated main calls nothing.
   *
   * When every function in a file is skipped by the harness (all params are
   * pointers/structs, etc.), MainGenConsumer emits an empty `main` whose body
   * is just `return 0;`. Such a benchmark exercises no code and should be
   * discarded. This is a post-write text check, coupled to MainGenConsumer's
   * generated-main format.
   *
   * @param path Path to the transformed C file to inspect.
   * @return true if the generated main contains no calls.
   */
  bool harnessIsEmpty(std::filesystem::path path);

  /**
   * @brief Parses the config file, populating {@code configuration}.
   *
   * Recognised keys: benchmarkDir, filterDir, debugLevel, keepCompilesOnly,
   * wipeOldBenchmarks. Unrecognised keys are ignored.
   *
   * @param configFile Path to the INI-style properties file.
   */
  void parseConfig(std::string configFile);

  /**
   * @brief Main entry point — runs transformAll over filterDir.
   *
   * @return The number of compilable benchmarks produced.
   */
  int run();

  /**
   * @brief Returns the set of verification properties for a benchmark.
   *
   * Currently returns a fixed set (termination + no-overflow) for every file.
   * Future: accepts AST characteristics and conditionally includes properties
   * (e.g. only termination if loops are present, only no-overflow if integer
   * arithmetic is present, unreach-call when {@code reach_error()} guards exist).
   *
   * @return Vector of properties to include in the task .yml.
   */
  std::vector<BenchmarkProperty> selectProperties();

  /**
   * @brief Writes an SV-Comp .yml task definition alongside the benchmark .c file.
   *
   * The task file references the preprocessed {@code .i} form of the input
   * (preprocessing is assumed as a later step). Properties are selected via
   * {@code selectProperties()}.
   *
   * @param cPath Path to the transformed .c benchmark file.
   */
  void writeBenchmarkTask(std::filesystem::path cPath);

  /**
   * @brief Preprocesses a transformed .c file into a .i file.
   *
   * Runs {@code gcc -E -P -std=gnu11} on the source file, writing the
   * preprocessed output alongside it with a {@code .i} extension.
   *
   * @param cPath Path to the transformed .c benchmark file.
   * @return true if preprocessing succeeded, false otherwise.
   */
  bool preprocess(std::filesystem::path cPath);

private:
  /// Path settings and flags loaded from the config file.
  struct transformConfigs configuration;
  /// Count of .c files attempted across the run, for the end-of-run summary.
  int _totalProcessed = 0;
};
