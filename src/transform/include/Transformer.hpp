#pragma once

#include <filesystem>
#include <string>

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

private:
  /// Path settings and flags loaded from the config file.
  struct transformConfigs configuration;
};
