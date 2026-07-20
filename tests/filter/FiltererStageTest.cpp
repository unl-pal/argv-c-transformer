// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Filterer.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

static std::string readFile(const fs::path &path) {
  std::ifstream in(path);
  std::stringstream buf;
  buf << in.rdbuf();
  return buf.str();
}

static void writeFile(const fs::path &path, const std::string &content) {
  fs::create_directories(path.parent_path());
  std::ofstream(path) << content;
}

// ---------------------------------------------------------------------------
// Stage-level tests for Filterer
// ---------------------------------------------------------------------------
//
// These exercise the full Filterer pipeline (config parsing -> pre-filter ->
// Clang consumer chain) against temporary directories: the pre-filter's LoC
// bounds and header gate, threshold-based body stripping, and the mirrored
// output layout. Counting/removal *decisions* are unit-tested in
// CountingVisitorTest.cpp and FilterFunctionsConsumerTest.cpp; this file
// checks what actually lands on disk.

class FiltererStageTest : public ::testing::Test {
protected:
  fs::path tmpDir;
  fs::path databaseDir;
  fs::path filterDir;
  fs::path configPath;

  void SetUp() override {
    tmpDir = fs::temp_directory_path() / ("filterer_stage_test_" + std::to_string(getpid()));
    databaseDir = tmpDir / "database";
    filterDir = tmpDir / "filtered";
    configPath = tmpDir / "test.config";
    fs::create_directories(databaseDir);
    fs::create_directories(filterDir);
  }

  void TearDown() override { fs::remove_all(tmpDir); }

  // Writes the config with common path settings plus any extra key lines.
  void writeConfig(const std::string &extra = "") {
    std::ofstream cfg(configPath);
    cfg << "[File Locations]\n"
        << "databaseDir = " << databaseDir.string() << "\n"
        << "filterDir = " << filterDir.string() << "\n"
        << "[Debugging Flags]\n"
        << "debugLevel = 0\n"
        << extra;
  }
};

TEST_F(FiltererStageTest, PassingFileIsCopiedToFilterDir) {
  writeConfig();
  writeFile(databaseDir / "simple.c", "int add(int a, int b) { return a + b; }\n");

  Filterer f(configPath.string());
  f.run();

  ASSERT_TRUE(fs::exists(filterDir / "simple.c"));
  EXPECT_NE(readFile(filterDir / "simple.c").find("return a + b;"), std::string::npos);
}

TEST_F(FiltererStageTest, MirrorsDirectoryStructure) {
  writeConfig();
  writeFile(databaseDir / "owner" / "repo" / "util.c", "int square(int x) { return x * x; }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_TRUE(fs::exists(filterDir / "owner" / "repo" / "util.c"));
}

TEST_F(FiltererStageTest, RejectsFileBelowMinLoC) {
  writeConfig("minFileLoC = 10\n");
  writeFile(databaseDir / "tiny.c", "int one(void) { return 1; }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_FALSE(fs::exists(filterDir / "tiny.c"));
}

TEST_F(FiltererStageTest, RejectsFileAboveMaxLoC) {
  writeConfig("maxFileLoC = 2\n");
  writeFile(databaseDir / "big.c",
            "int a(void) { return 1; }\n"
            "int b(void) { return 2; }\n"
            "int c(void) { return 3; }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_FALSE(fs::exists(filterDir / "big.c"));
}

TEST_F(FiltererStageTest, RejectsNonStdHeaderByDefault) {
  // useNonStdHeaders defaults to 0: a project-local include disqualifies
  // the whole file at the pre-filter, before any Clang parsing.
  writeConfig();
  writeFile(databaseDir / "uses_local.c",
            "#include \"project.h\"\n"
            "int f(void) { return 0; }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_FALSE(fs::exists(filterDir / "uses_local.c"));
}

TEST_F(FiltererStageTest, AcceptsStdHeader) {
  writeConfig();
  writeFile(databaseDir / "uses_std.c",
            "#include <string.h>\n"
            "int f(void) { return 0; }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_TRUE(fs::exists(filterDir / "uses_std.c"));
}

TEST_F(FiltererStageTest, AcceptsNonStdHeaderWhenEnabled) {
  writeConfig("useNonStdHeaders = true\n");
  writeFile(databaseDir / "project.h", "int helper(void);\n");
  writeFile(databaseDir / "uses_local.c",
            "#include \"project.h\"\n"
            "int f(void) { return helper(); }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_TRUE(fs::exists(filterDir / "uses_local.c"));
}

TEST_F(FiltererStageTest, StripsFunctionFailingComplexityThreshold) {
  // ForLoops = 1,99999 requires at least one for loop per function: `plain`
  // fails and is stripped to a bare declaration; `loopy` keeps its body.
  writeConfig("ForLoops = 1,99999\n");
  writeFile(databaseDir / "mixed.c",
            "int loopy(int n) {\n"
            "  int s = 0;\n"
            "  for (int i = 0; i < n; i++) s += i;\n"
            "  return s;\n"
            "}\n"
            "int plain(int x) { return x + 1; }\n");

  Filterer f(configPath.string());
  f.run();

  ASSERT_TRUE(fs::exists(filterDir / "mixed.c"));
  std::string out = readFile(filterDir / "mixed.c");
  EXPECT_NE(out.find("for (int i = 0; i < n; i++)"), std::string::npos) << out;
  EXPECT_NE(out.find("int plain(int x) ;"), std::string::npos) << out;
  EXPECT_EQ(out.find("return x + 1;"), std::string::npos) << out;
}
