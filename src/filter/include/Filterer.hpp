#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

/**
 * @brief Runtime configuration loaded from the INI-style config file.
 *
 * Holds the path settings and flags that live outside the numeric threshold
 * map — i.e. things that are not per-function min/max counts.
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
   * @brief Parses the config file, populating the threshold map and type lists.
   *
   * Numeric thresholds go into {@code config}. The special {@code type} key
   * populates {@code typesRequested} and {@code typeNames}. Path settings go
   * into {@code configuration}. Unknown keys are reported to stdout.
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
   * @brief Prints a debug message if debug mode is enabled.
   *
   * @param info Message to print.
   */
  void debugInfo(std::string info);

  /**
   * @brief Main entry point — collects files, pre-filters, and runs the AST pipeline.
   *
   * For each .c file that passes {@code checkPotentialFile}, builds a Clang
   * tool invocation and runs the full filter consumer chain (count → filter →
   * remove → inject verifiers), writing the result to filterDir.
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

  /// Clang BuiltinType enum values for each type requested via the config's {@code type} key.
  std::vector<unsigned int> typesRequested;

  /// Human-readable names parallel to typesRequested (same index = same type).
  std::vector<std::string> typeNames;

  /**
   * @brief Per-function and per-file numeric thresholds loaded from the config.
   *
   * Keys follow the pattern min/max + metric name (e.g. minForLoops,
   * maxFileLoC). Defaults are 0 for minimums and 99999 for maximums, meaning
   * no filtering unless explicitly configured.
   */
  std::map<std::string, int> config = {{"debugLevel", 0},
                                       {"maxCallFunc", 99999},
                                       {"maxFileLoC", 99999},
                                       {"maxForLoops", 99999},
                                       {"maxFunctions", 99999},
                                       {"maxIfStmt", 99999},
                                       {"maxParam", 99999},
                                       {"maxTypeArithmeticOperation", 99999},
                                       {"maxTypeCompareOperation", 99999},
                                       {"maxTypeIfStmt", 99999},
                                       {"maxTypeParameters", 99999},
                                       {"maxTypePostfix", 99999},
                                       {"maxTypePrefix", 99999},
                                       {"maxTypeUnaryOperation", 99999},
                                       {"maxTypeVariableReference", 99999},
                                       {"maxTypeVariables", 99999},
                                       {"maxWhileLoops", 99999},
                                       {"minCallFunc", 0},
                                       {"minFileLoC", 0},
                                       {"minForLoops", 0},
                                       {"minFunctions", 0},
                                       {"minIfStmt", 0},
                                       {"minParam", 0},
                                       {"minTypeArithmeticOperation", 0},
                                       {"minTypeCompareOperation", 0},
                                       {"minTypeIfStmt", 0},
                                       {"minTypeParameters", 0},
                                       {"minTypePostfix", 0},
                                       {"minTypePrefix", 0},
                                       {"minTypeUnaryOperation", 0},
                                       {"minTypeVariableReference", 0},
                                       {"minTypeVariables", 0},
                                       {"minWhileLoops", 0},
                                       {"useNonStdHeaders", 0}};

  /// Path settings and flags that don't fit the numeric threshold map.
  struct filterConfigs configuration;
};
