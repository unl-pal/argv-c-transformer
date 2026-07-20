// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "TransformAction.hpp"

#include "ClangToolUtils.hpp"

#include <algorithm>
#include <cctype>
#include <clang/Serialization/PCHContainerOperations.h>
#include <clang/Tooling/Tooling.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Golden-file tests for the transform pipeline
// ---------------------------------------------------------------------------
//
// Each test case is a pair of files in tests/transform/cases/:
//   <name>.input.c     — source fed through the full TransformAction pipeline
//                        (include stripping -> call havocking -> main
//                        generation -> verifier extern injection)
//   <name>.expected.c  — the exact output the pipeline must produce
//
// Support headers (plain .h files) live alongside the cases and resolve
// through the real filesystem, so #include "..." behaves as in the real
// pipeline.
//
// To regenerate goldens after an intentional behavior change:
//   UPDATE_GOLDENS=1 ./build/tests/transform_tests
// then review the .expected.c diffs like any other code change.

struct GoldenCase {
  std::string name;
  fs::path input;
  fs::path expected;
};

// Without this, gtest (and ctest's test discovery) prints the param as a raw
// byte dump in test listings
static void PrintTo(const GoldenCase &testCase, std::ostream *os) { *os << testCase.name; }

static std::vector<GoldenCase> discoverCases() {
  const std::string suffix = ".input.c";
  std::vector<GoldenCase> cases;
  for (const fs::directory_entry &entry : fs::directory_iterator(TRANSFORM_TEST_CASES_DIR)) {
    std::string fileName = entry.path().filename().string();
    if (fileName.size() <= suffix.size() ||
        fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) != 0)
      continue;
    std::string name = fileName.substr(0, fileName.size() - suffix.size());
    cases.push_back({name, entry.path(), entry.path().parent_path() / (name + ".expected.c")});
  }
  std::sort(cases.begin(), cases.end(),
            [](const GoldenCase &a, const GoldenCase &b) { return a.name < b.name; });
  return cases;
}

static std::string readFile(const fs::path &path) {
  std::ifstream in(path);
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

class TransformGolden : public ::testing::TestWithParam<GoldenCase> {};

TEST_P(TransformGolden, MatchesExpected) {
  const GoldenCase &testCase = GetParam();
  std::string code = readFile(testCase.input);
  ASSERT_FALSE(code.empty()) << "could not read " << testCase.input;

  // System headers need a usable clang resource dir; skip with a pointer at
  // the misconfiguration rather than failing on a missing stddef.h.
  std::optional<std::string> resourceDir = getResourceDir();
  if (code.find("#include <") != std::string::npos) {
    if (!resourceDir)
      GTEST_SKIP() << "CLANG_RESOURCES not set; case needs system headers";
    if (!fs::exists(fs::path(*resourceDir) / "include" / "stddef.h"))
      GTEST_SKIP() << "CLANG_RESOURCES=" << *resourceDir
                   << " has no include/stddef.h (stale after a clang upgrade?); "
                      "re-export with: export CLANG_RESOURCES=$(clang -print-resource-dir)";
  }

  std::vector<std::string> args = {"-xc"};
  if (resourceDir)
    args.push_back("-resource-dir=" + *resourceDir);
  std::optional<std::string> sysroot = getSysroot();
  if (sysroot) {
    args.push_back("-isysroot");
    args.push_back(*sysroot);
  }

  // The input's real path is used as the tool's file name so that quoted
  // includes resolve against the cases directory.
  std::string out;
  llvm::raw_string_ostream os(out);
  bool ok = clang::tooling::runToolOnCodeWithArgs(
      std::make_unique<TransformAction>(os), code, args, testCase.input.string(),
      "transform-test", std::make_shared<clang::PCHContainerOperations>());
  ASSERT_TRUE(ok) << "transform tool failed for " << testCase.input;

  if (std::getenv("UPDATE_GOLDENS")) {
    std::ofstream(testCase.expected) << out;
    GTEST_SKIP() << "golden updated: " << testCase.expected;
  }

  ASSERT_TRUE(fs::exists(testCase.expected))
      << "missing golden " << testCase.expected << "; run with UPDATE_GOLDENS=1 to create it";
  EXPECT_EQ(readFile(testCase.expected), out) << "output diverges from " << testCase.expected;
}

static std::string caseName(const ::testing::TestParamInfo<GoldenCase> &info) {
  std::string name = info.param.name;
  std::replace_if(
      name.begin(), name.end(),
      [](unsigned char c) { return !std::isalnum(c); }, '_');
  return name;
}

INSTANTIATE_TEST_SUITE_P(Cases, TransformGolden, ::testing::ValuesIn(discoverCases()), caseName);
