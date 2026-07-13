// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/Basic/Version.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <optional>
#include <regex>
#include <string>
#include <vector>

/**
 * @brief Aborts the process when built against an unsupported Clang.
 *
 * LLVM 20 is the minimum; older versions miss APIs this project relies on and
 * have produced AST-traversal crashes. Newer versions are accepted.
 */
inline void checkClangVersion() {
  if (CLANG_VERSION_MAJOR < 20) {
    std::cerr << "Error: built against Clang " << CLANG_VERSION_STRING
              << ", but Clang 20 or newer is required. "
              << "Specify the Clang version for CMake per README.md." << std::endl;
    std::exit(1);
  }
}

/**
 * @brief Runs a shell command and returns its stdout, stripped of the
 * trailing newline.
 *
 * @return The captured output, or std::nullopt if the command could not be
 *         run or produced no output.
 */
inline std::optional<std::string> readCommandOutput(const char *cmd) {
  FILE *pipe = popen(cmd, "r");
  if (!pipe)
    return std::nullopt;
  char buf[512];
  std::string result;
  while (fgets(buf, sizeof(buf), pipe))
    result += buf;
  pclose(pipe);
  if (!result.empty() && result.back() == '\n')
    result.pop_back();
  return result.empty() ? std::nullopt : std::optional<std::string>(result);
}

/**
 * @brief Returns the macOS SDK sysroot, if applicable.
 *
 * On macOS, system C headers (string.h, stdlib.h, …) live inside the SDK
 * rather than /usr/include.  Returns std::nullopt on non-Apple platforms or
 * if xcrun fails.
 */
inline std::optional<std::string> getSysroot() {
#ifndef __APPLE__
  return std::nullopt;
#else
  return readCommandOutput("xcrun --show-sdk-path 2>/dev/null");
#endif
}

/**
 * @brief Returns the clang resource directory.
 *
 * Checks CLANG_RESOURCES first (lets callers override, e.g. for cross-compile
 * setups). Falls back to running `clang -print-resource-dir` so the binary
 * is self-configuring on systems where the env var isn't set.
 */
inline std::optional<std::string> getResourceDir() {
  const char *r = std::getenv("CLANG_RESOURCES");
  if (r)
    return std::string(r);
  return readCommandOutput("clang -print-resource-dir 2>/dev/null");
}

/**
 * @brief Builds the standard argument list for a single-file ClangTool invocation.
 *
 * Returns owned {@code std::string} values rather than {@code const char*} so
 * the caller controls storage lifetime. Pass the result to
 * {@code CommonOptionsParser::create} via a {@code vector<const char*>} view.
 * The compile flags are placed after a literal {@code --}, which makes
 * {@code CommonOptionsParser} build a {@code FixedCompilationDatabase}
 * directly instead of searching {@code filePath}'s ancestor directories for a
 * {@code compile_commands.json}.
 * 
 * @param filePath    Path to the C source file to process.
 * @param resourceDir Value of {@code CLANG_RESOURCES} (from {@code getResourceDir}).
 * @return Argument vector suitable for passing to {@code CommonOptionsParser::create}.
 */
inline std::vector<std::string> buildClangArgs(const std::string &filePath,
                                               const std::string &resourceDir) {
  std::vector<std::string> args = {
      "clang",
      filePath,
      "--",
      "-xc",
      "-resource-dir=" + resourceDir,
      "-fparse-all-comments",
  };
  std::optional<std::string> sysroot = getSysroot();
  if (sysroot) {
    args.push_back("-isysroot");
    args.push_back(*sysroot);
  }
  return args;
}

/**
 * @brief Builds a null-terminated {@code argv}-style view over owned argument strings.
 *
 * The returned vector holds {@code .c_str()} pointers into {@code args}, plus
 * a trailing {@code nullptr}, ready to pass to
 * {@code CommonOptionsParser::create(argc, argv.data(), ...)}. The caller
 * must keep {@code args} alive for as long as the returned view is used.
 *
 * @param args Owned argument strings (e.g. from {@code buildClangArgs}).
 * @return A {@code const char*} view over {@code args}, terminated by {@code nullptr}.
 */
inline std::vector<const char *> toArgv(const std::vector<std::string> &args) {
  std::vector<const char *> argv;
  argv.reserve(args.size() + 1);
  for (const std::string &arg : args)
    argv.push_back(arg.c_str());
  argv.push_back(nullptr);
  return argv;
}

/**
 * @brief Runs a FrontendActionFactory over a single C file with the standard
 * tool setup shared by the filter and transform steps.
 *
 * Bundles the boilerplate both drivers need: resource-dir discovery,
 * argument building, options parsing, and a diagnostics-suppressing
 * ClangTool. Tool-reported errors (the file may be arbitrary downloaded C)
 * are logged but still count as a successful run — the rewritten output is
 * written regardless, and downstream compile checks decide its fate.
 *
 * @param filePath Path to the C source file to process.
 * @param factory  Factory producing the FrontendAction to run.
 * @return true if the tool ran; false if setup failed (no resource dir,
 *         unparsable options).
 */
inline bool runToolOnFile(const std::string &filePath,
                          clang::tooling::FrontendActionFactory &factory) {
  static llvm::cl::OptionCategory toolCategory("argv-c-transformer");
  clang::IgnoringDiagConsumer diagConsumer;

  std::optional<std::string> resourceDir = getResourceDir();
  if (!resourceDir) {
    std::cerr << "Could not determine clang resource directory (set CLANG_RESOURCES to override)"
              << std::endl;
    return false;
  }

  std::vector<std::string> args = buildClangArgs(filePath, *resourceDir);
  std::vector<const char *> argv = toArgv(args);
  int argc = static_cast<int>(args.size());

  llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser =
      clang::tooling::CommonOptionsParser::create(argc, argv.data(), toolCategory);
  if (!expectedParser) {
    llvm::errs() << expectedParser.takeError();
    return false;
  }
  clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();

  clang::tooling::ClangTool tool(optionsParser.getCompilations(),
                                 optionsParser.getSourcePathList());
  tool.setDiagnosticConsumer(&diagConsumer);
  try {
    if (tool.run(&factory) != 0)
      std::cerr << "Clang tool reported errors while processing: " << filePath << std::endl;
  } catch (const std::exception &e) {
    // A consumer bug (e.g. a config/metric name mismatch) should not take
    // down the whole batch — report it and move on to the next file.
    std::cerr << "Clang tool threw while processing " << filePath << ": " << e.what() << std::endl;
    return false;
  }
  return true;
}

/**
 * @brief Strips leading and trailing spaces/tabs.
 */
inline std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t");
  size_t end = s.find_last_not_of(" \t");
  if (start == std::string::npos)
    return "";
  return s.substr(start, end - start + 1);
}

/**
 * @brief Parses an INI-style config file and returns raw key/value string pairs.
 *
 * Lines that do not match the {@code key = value} pattern (comments, blank
 * lines, section headers) are silently skipped. Each tool is responsible for
 * interpreting its own keys from the returned map; unknown keys are not
 * reported here.
 *
 * @param configFile Path to the INI-style properties file.
 * @return Map of key to raw string value for every matched line, or an empty
 *         map if the file does not exist or cannot be opened.
 */
inline std::map<std::string, std::string> parseIniFile(const std::string &configFile) {
  std::map<std::string, std::string> result;
  if (!std::filesystem::exists(configFile))
    return result;
  std::ifstream file(configFile);
  if (!file.is_open())
    return result;
  std::regex pattern(R"(^\s*(\w+)\s*=\s*([0-9]+|[\w\s,\-]+|[\w/\-_.]+)$)");
  std::string line;
  std::smatch match;
  while (std::getline(file, line)) {
    if (std::regex_search(line, match, pattern))
      result[match[1]] = match[2];
  }
  return result;
}
