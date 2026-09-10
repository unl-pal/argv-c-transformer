// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "DebugLog.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/Version.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
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

/** @brief Returns the macOS SDK sysroot (system C headers live inside the SDK there, not /usr/include), or nullopt on non-Apple platforms or if xcrun fails. */
inline std::optional<std::string> getSysroot() {
#ifndef __APPLE__
  return std::nullopt;
#else
  return readCommandOutput("xcrun --show-sdk-path 2>/dev/null");
#endif
}

/** @brief Returns the major version of the clang on PATH, or nullopt if there isn't one or its version is unparseable. */
inline std::optional<int> getPathClangMajorVersion() {
  std::optional<std::string> version = readCommandOutput("clang -dumpversion 2>/dev/null");
  if (!version)
    return std::nullopt;
  try {
    return std::stoi(*version);
  } catch (...) {
    return std::nullopt;
  }
}

/**
 * @brief Returns the clang resource directory (builtin headers), preferring
 * CLANG_RESOURCES or PATH's clang if it matches CLANG_VERSION_MAJOR, else
 * falling back to whichever is available with a warning.
 */
inline std::optional<std::string> getResourceDir() {
  const char *r = std::getenv("CLANG_RESOURCES");
  std::optional<std::string> envResourceDir = r ? std::optional<std::string>(std::string(r)) : std::nullopt;

  if (envResourceDir) {
    std::string basename = std::filesystem::path(*envResourceDir).filename().string();
    try {
      if (std::stoi(basename) == CLANG_VERSION_MAJOR)
        return envResourceDir;
      debugLog(0, "CLANG_RESOURCES (" + *envResourceDir + ") looks like Clang " + basename +
                  ", but this was built against Clang " + std::to_string(CLANG_VERSION_MAJOR) +
                  ". Preferring a matching clang on PATH if one is found.");
    } catch (...) {
      // Non-numeric basename; can't validate it, so trust it as given.
      return envResourceDir;
    }
  }

  std::optional<int> pathVersion = getPathClangMajorVersion();
  if (pathVersion) {
    if (*pathVersion != CLANG_VERSION_MAJOR)
      debugLog(0, "`clang` on PATH is version " + std::to_string(*pathVersion) +
                  ", but this was built against Clang " + std::to_string(CLANG_VERSION_MAJOR) + ".");
    if (*pathVersion == CLANG_VERSION_MAJOR || !envResourceDir) {
      std::optional<std::string> pathResourceDir = readCommandOutput("clang -print-resource-dir 2>/dev/null");
      if (pathResourceDir) {
        if (*pathVersion != CLANG_VERSION_MAJOR)
          debugLog(0, "Using its resource directory anyway (" + *pathResourceDir +
                      "); its builtin headers may not match this build.");
        return pathResourceDir;
      }
    }
  }

  if (envResourceDir) {
    debugLog(0, "Using CLANG_RESOURCES anyway (" + *envResourceDir +
                "); its builtin headers may not match this build.");
    return envResourceDir;
  }

  debugLog(0, "No `clang` found on PATH to supply the resource directory (builtin "
              "headers). Set CLANG_RESOURCES to point at one, or put a clang on PATH.");
  return std::nullopt;
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
 * @brief Runs a FrontendActionFactory over a single C file in-process and
 * reports whether it completed with zero diagnosed errors.
 *
 * @param filePath      Path to the C source file to process.
 * @param extraArgs     Extra driver-style args appended after the standard
 *                      "-xc -resource-dir=... [-isysroot ...]" flags (e.g.
 *                      {"-E", "-P", "-o", iPath}).
 * @param factory       Factory producing the FrontendAction to run.
 * @param diagnosticsOut If non-null, filled with the formatted diagnostic text.
 * @return true if the tool ran and diagnosed zero errors.
 */
inline bool runFrontendActionCheckingErrors(const std::string &filePath,
                                            const std::vector<std::string> &extraArgs,
                                            clang::tooling::FrontendActionFactory &factory,
                                            std::string *diagnosticsOut = nullptr) {
  std::optional<std::string> resourceDir = getResourceDir();
  if (!resourceDir) {
    debugLog(0, "Could not determine clang resource directory (set CLANG_RESOURCES to override)");
    return false;
  }

  std::vector<std::string> flags = {"-xc", "-resource-dir=" + *resourceDir, "-fparse-all-comments"};
  if (std::optional<std::string> sysroot = getSysroot()) {
    flags.push_back("-isysroot");
    flags.push_back(*sysroot);
  }
  flags.insert(flags.end(), extraArgs.begin(), extraArgs.end());

  clang::tooling::FixedCompilationDatabase compilations(".", flags);
  clang::tooling::ClangTool tool(compilations, {filePath});
  tool.clearArgumentsAdjusters(); // clear -fsyntax-only

  std::string diagnosticText;
  llvm::raw_string_ostream diagStream(diagnosticText);
  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> diagOpts(new clang::DiagnosticOptions());
  clang::TextDiagnosticPrinter diagPrinter(diagStream, diagOpts.get());
  tool.setDiagnosticConsumer(&diagPrinter);

  try {
    tool.run(&factory);
  } catch (const std::exception &e) {
    debugLog(0, "Clang tool threw while processing " + filePath + ": " + e.what());
    return false;
  }
  if (diagnosticsOut)
    *diagnosticsOut = diagStream.str();
  return diagPrinter.getNumErrors() == 0;
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
