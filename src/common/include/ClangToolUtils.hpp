#pragma once

#include <clang/Basic/Version.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <string>
#include <vector>

inline void checkClangVersion() {
  if (CLANG_VERSION_MAJOR != 20) {
    std::cerr << "Warning: built against Clang " << CLANG_VERSION_STRING
              << ", expected Clang 20. Rebuild with the correct LLVM version."
              << "Specify Clang version for Cmake per README.md" << std::endl;
  }
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
  FILE *pipe = popen("xcrun --show-sdk-path 2>/dev/null", "r");
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

  FILE *pipe = popen("clang -print-resource-dir 2>/dev/null", "r");
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
 * @brief Builds the standard argument list for a single-file ClangTool invocation.
 *
 * Returns owned {@code std::string} values rather than {@code const char*} so
 * the caller controls storage lifetime. Pass the result to
 * {@code CommonOptionsParser::create} via a {@code vector<const char*>} view.
 *
 * @param filePath    Path to the C source file to process.
 * @param resourceDir Value of {@code CLANG_RESOURCES} (from {@code getResourceDir}).
 * @return Argument vector suitable for passing to {@code CommonOptionsParser::create}.
 */
inline std::vector<std::string> buildClangArgs(const std::string &filePath,
                                               const std::string &resourceDir) {
  std::vector<std::string> args = {
      "clang",
      "-extra-arg=-xc",
      "-extra-arg=-resource-dir=" + resourceDir,
      "-extra-arg=-fparse-all-comments",
  };
  std::optional<std::string> sysroot = getSysroot();
  if (sysroot) {
    args.push_back("-extra-arg=-isysroot");
    args.push_back("-extra-arg=" + *sysroot);
  }
  args.push_back(filePath);
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
  std::regex pattern(R"(^\s*(\w+)\s*=\s*([0-9]+|[\w\s,]+|[\w/\-_.]+)$)");
  std::string line;
  std::smatch match;
  while (std::getline(file, line)) {
    if (std::regex_search(line, match, pattern))
      result[match[1]] = match[2];
  }
  return result;
}
