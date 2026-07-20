// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

/**
 * @brief What the user asked for on the command line.
 *
 * Either field may be empty: an empty inputPath means "use the directories
 * from the config / defaults", an empty configFile means "run with built-in
 * defaults".
 */
struct CliInvocation {
  std::string inputPath;  ///< Directory or single .c file to process ("" = from config).
  std::string configFile; ///< INI-style properties file ("" = defaults only).
};

/**
 * @brief Classifies positional arguments into input path vs config file.
 *
 * A directory or a .c file is unambiguously an input; anything else is
 * treated as a config file.
 *
 * @param argc argc from main.
 * @param argv argv from main.
 * @return The parsed invocation, or std::nullopt if parsing fails
 */
inline std::optional<CliInvocation> parseCliArgs(int argc, char **argv) {
  if (argc < 2 || argc > 3)
    return std::nullopt;

  CliInvocation invocation;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    std::filesystem::path path(arg);
    bool isInput = std::filesystem::is_directory(path) ||
                   (path.extension() == ".c" && std::filesystem::is_regular_file(path));
    std::string &slot = isInput ? invocation.inputPath : invocation.configFile;
    if (!slot.empty()) {
      std::cerr << "Both '" << slot << "' and '" << arg << "' look like "
                << (isInput ? "inputs" : "config files") << " — give at most one of each."
                << std::endl;
      return std::nullopt;
    }
    slot = arg;
  }
  return invocation;
}

/**
 * @brief Prints the shared usage message for the filter/transform/full tools.
 *
 * @param toolName Binary name to show in the examples (e.g. "filter").
 */
inline void printUsage(const std::string &toolName) {
  std::cerr << "Usage: " << toolName << " [<input>] [<config-file>]\n"
            << "  <input>       directory of C files, or a single .c file\n"
            << "  <config-file> INI-style properties file (see properties.config)\n"
            << "At least one argument is required; with only an input, built-in\n"
            << "defaults are used and output goes to <name>-filtered/ etc. in the\n"
            << "working directory. If both are given input supercedes a repo\n"
            << "defined in the properties file."
            << "Examples:\n"
            << "  ./build/" << toolName << " path/to/repo\n"
            << "  ./build/" << toolName << " foo.c\n"
            << "  ./build/" << toolName << " properties.config\n"
            << "  ./build/" << toolName << " path/to/repo properties.config" << std::endl;
}

/**
 * @brief Derives the short name of an input for output-directory naming.
 *
 * A directory yields its final component ("path/to/redis" → "redis"), a file
 * its stem ("src/foo.c" → "foo"). A trailing "-filtered" suffix is stripped so
 * the transform step run on "redis-filtered" still names its output
 * "redis-benchmarks" rather than "redis-filtered-benchmarks".
 *
 * @param inputPath Directory or .c file path as given on the command line.
 * @return The base name to build "<name>-filtered" / "<name>-benchmarks" from.
 */
inline std::string inputBaseName(const std::string &inputPath) {
  // lexically_normal drops a trailing slash so "repo/" still yields "repo".
  std::filesystem::path path = std::filesystem::path(inputPath).lexically_normal();
  std::string name = path.stem().string();
  if (name.empty() || name == ".")
    name = std::filesystem::absolute(path).parent_path().filename().string();
  const std::string suffix = "-filtered";
  if (name.size() > suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
    name = name.substr(0, name.size() - suffix.size());
  return name;
}
