// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "DebugLog.hpp"

#include <clang/Basic/Version.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

/** @brief Aborts if built against Clang < 20: older versions miss APIs this project relies on and have caused AST-traversal crashes. */
inline void checkClangVersion() {
  if (CLANG_VERSION_MAJOR < 20) {
    std::cerr << "Error: built against Clang " << CLANG_VERSION_STRING
              << ", but Clang 20 or newer is required. "
              << "Specify the Clang version for CMake per README.md." << std::endl;
    std::exit(1);
  }
}

/** @brief Runs a shell command, returns its stdout stripped of the trailing newline, or nullopt if it couldn't run or produced no output. */
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
 * @brief Aborts if the `clang` resolved off PATH at runtime is too old.
 *
 * checkClangVersion() only confirms the Clang this binary was *built*
 * against; clangCommand() shells out to a bare "clang" resolved independently
 * by PATH, which can silently be a different, older install.
 */
inline void checkRuntimeClangVersion() {
  std::optional<std::string> version = readCommandOutput("clang -dumpversion 2>/dev/null");
  if (!version) {
    std::cerr << "Error: no `clang` found on PATH. This project needs one at runtime "
                 "for preprocessing and compile-checking. See README.md (\"Dependencies\") "
                 "for how to get one."
              << std::endl;
    std::exit(1);
  }
  int major;
  try {
    major = std::stoi(*version);
  } catch (...) {
    return; // Unparseable output; let it through and surface any real issue later.
  }
  if (major < 20) {
    std::cerr << "Error: `clang` on PATH is version " << *version
              << ", but 20 or newer is required. See README.md (\"clang on PATH must "
                 "match the build\") for how to fix this."
              << std::endl;
    std::exit(1);
  }
}

/** @brief Returns the macOS SDK sysroot (system C headers live inside the SDK there, not /usr/include), or nullopt on non-Apple platforms or if xcrun fails. */
inline std::optional<std::string> getSysroot() {
#ifndef __APPLE__
  return std::nullopt;
#else
  return readCommandOutput("xcrun --show-sdk-path 2>/dev/null");
#endif
}

/** @brief Returns the clang resource directory: CLANG_RESOURCES if set, else `clang -print-resource-dir`. */
inline std::optional<std::string> getResourceDir() {
  const char *r = std::getenv("CLANG_RESOURCES");
  if (r)
    return std::string(r);
  return readCommandOutput("clang -print-resource-dir 2>/dev/null");
}

/**
 * @brief Builds "clang <flags> -resource-dir=<dir> [-isysroot <sdk>]" for a std::system shell-out.
 * @param flags Compiler flags placed right after "clang" (e.g. "-E -P").
 * @return The command string, or nullopt if the resource directory can't be determined.
 */
inline std::optional<std::string> clangCommand(const std::string &flags) {
  std::optional<std::string> resourceDir = getResourceDir();
  if (!resourceDir)
    return std::nullopt;
  std::string cmd = "clang " + flags + " -resource-dir=" + *resourceDir;
  if (std::optional<std::string> sysroot = getSysroot())
    cmd += " -isysroot " + *sysroot;
  return cmd;
}

/**
 * @brief Builds the standard argument list for a single-file ClangTool invocation.
 *
 * The compile flags are placed after a literal {@code --}, which makes
 * {@code CommonOptionsParser} build a {@code FixedCompilationDatabase}
 * directly instead of searching for a {@code compile_commands.json}.
 *
 * @param filePath        Path to the C source file to process.
 * @param resourceDir     Value from {@code getResourceDir}.
 * @param extraIncludeDirs Directories to add as -I search paths (e.g. from
 *                         {@code collectLocalIncludeDirs}), so quoted
 *                         #includes the preprocessor's default search
 *                         (current file's directory, then system paths)
 *                         wouldn't otherwise find can still resolve.
 * @return Argument vector for {@code CommonOptionsParser::create}.
 */
inline std::vector<std::string> buildClangArgs(const std::string &filePath,
                                               const std::string &resourceDir,
                                               const std::vector<std::string> &extraIncludeDirs = {}) {
  std::vector<std::string> args = {
      "clang",
      filePath,
      "--",
      "-xc",
      "-resource-dir=" + resourceDir,
      "-fparse-all-comments",
  };
  for (const std::string &dir : extraIncludeDirs)
    args.push_back("-I" + dir);
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
 * Caller must keep {@code args} alive for as long as the returned view is used.
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
 * Tool-reported errors (the file may be arbitrary downloaded C) are logged
 * but still count as a successful run; downstream compile checks decide the
 * output's fate.
 *
 * @param filePath        Path to the C source file to process.
 * @param factory         Factory producing the FrontendAction to run.
 * @param extraIncludeDirs Directories to add as -I search paths; see {@code buildClangArgs}.
 * @return true if the tool ran; false if setup failed (no resource dir, unparsable options).
 */
inline bool runToolOnFile(const std::string &filePath,
                          clang::tooling::FrontendActionFactory &factory,
                          const std::vector<std::string> &extraIncludeDirs = {}) {
  static llvm::cl::OptionCategory toolCategory("argv-c-transformer");
  clang::IgnoringDiagConsumer diagConsumer;

  std::optional<std::string> resourceDir = getResourceDir();
  if (!resourceDir) {
    debugLog(0, "Could not determine clang resource directory (set CLANG_RESOURCES to override)");
    return false;
  }

  std::vector<std::string> args = buildClangArgs(filePath, *resourceDir, extraIncludeDirs);
  std::vector<const char *> argv = toArgv(args);
  int argc = static_cast<int>(args.size());

  llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser =
      clang::tooling::CommonOptionsParser::create(argc, argv.data(), toolCategory);
  if (!expectedParser) {
    debugLog(0, "CommonOptionsParser::create failed for " + filePath + ": " +
                    llvm::toString(expectedParser.takeError()));
    return false;
  }
  clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();

  clang::tooling::ClangTool tool(optionsParser.getCompilations(),
                                 optionsParser.getSourcePathList());
  tool.setDiagnosticConsumer(&diagConsumer);
  try {
    if (tool.run(&factory) != 0)
      debugLog(1, "Clang tool reported errors while processing: " + filePath);
  } catch (const std::exception &e) {
    // A consumer bug (e.g. a config/metric name mismatch) should not take
    // down the whole batch. Report it and move on to the next file.
    debugLog(0, "Clang tool threw while processing " + filePath + ": " + e.what());
    return false;
  }
  return true;
}

/**
 * @brief Detects a trivial benchmark whose generated main calls nothing.
 *
 * Matches the exact main body MainGenConsumer emits when it harnesses
 * nothing, and that HarnessRepairConsumer's line-erasure collapses to when
 * every harness call is later repaired away. Coupled to both format strings.
 *
 * @param path Path to the generated C file to inspect.
 * @return true if the generated main contains no calls.
 */
inline bool harnessIsEmpty(std::filesystem::path path) {
  std::ifstream in(path);
  if (!in)
    return false;
  std::stringstream buffer;
  buffer << in.rdbuf();
  std::string content = buffer.str();
  return content.find("int main(void) {\n  return 0;\n}") != std::string::npos;
}

/** @brief Formats a duration as "Xm YY.Ys" (minutes elided under 60s), for stage summary output. */
inline std::string formatElapsed(std::chrono::steady_clock::duration d) {
  double totalSeconds = std::chrono::duration<double>(d).count();
  int minutes = static_cast<int>(totalSeconds) / 60;
  double seconds = totalSeconds - minutes * 60;
  std::ostringstream oss;
  if (minutes > 0)
    oss << minutes << "m ";
  oss << std::fixed << std::setprecision(minutes > 0 ? 1 : 2) << seconds << "s";
  return oss.str();
}
