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
        << "[Debug]\n"
        << "debugLevel = 0\n"
        << extra;
  }
};

TEST_F(FiltererStageTest, PassingFileIsCopiedToFilterDir) {
  writeConfig();
  writeFile(databaseDir / "simple.c", "int add(int a, int b) { return a + b; }\n");

  Filterer f(configPath.string());
  f.run();

  ASSERT_TRUE(fs::exists(filterDir / "simple__add.c"));
  EXPECT_NE(readFile(filterDir / "simple__add.c").find("return a + b;"), std::string::npos);
}

TEST_F(FiltererStageTest, MirrorsDirectoryStructure) {
  writeConfig();
  writeFile(databaseDir / "owner" / "repo" / "util.c", "int square(int x) { return x * x; }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_TRUE(fs::exists(filterDir / "owner" / "repo" / "util__square.c"));
}

TEST_F(FiltererStageTest, EachSurvivingFunctionGetsItsOwnOutput) {
  // The point of splitting: two independent functions in one source file
  // become two independent filtered outputs, each with only one live body.
  writeConfig();
  writeFile(databaseDir / "multi.c", "int add(int a, int b) { return a + b; }\n"
                                     "int sub(int a, int b) { return a - b; }\n");

  Filterer f(configPath.string());
  f.run();

  ASSERT_TRUE(fs::exists(filterDir / "multi__add.c"));
  ASSERT_TRUE(fs::exists(filterDir / "multi__sub.c"));

  std::string addOut = readFile(filterDir / "multi__add.c");
  EXPECT_NE(addOut.find("return a + b;"), std::string::npos) << addOut;
  EXPECT_NE(addOut.find("int sub(int a, int b) ;"), std::string::npos) << addOut;
  EXPECT_EQ(addOut.find("return a - b;"), std::string::npos) << addOut;

  std::string subOut = readFile(filterDir / "multi__sub.c");
  EXPECT_NE(subOut.find("return a - b;"), std::string::npos) << subOut;
  EXPECT_NE(subOut.find("int add(int a, int b) ;"), std::string::npos) << subOut;
  EXPECT_EQ(subOut.find("return a + b;"), std::string::npos) << subOut;
}

TEST_F(FiltererStageTest, RejectsFileBelowMinLoC) {
  writeConfig("FileLoC = 10\n");
  writeFile(databaseDir / "tiny.c", "int one(void) { return 1; }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_FALSE(fs::exists(filterDir / "tiny.c"));
}

TEST_F(FiltererStageTest, RejectsFileAboveMaxLoC) {
  writeConfig("FileLoC = ,2\n");
  writeFile(databaseDir / "big.c", "int a(void) { return 1; }\n"
                                   "int b(void) { return 2; }\n"
                                   "int c(void) { return 3; }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_FALSE(fs::exists(filterDir / "big.c"));
}

TEST_F(FiltererStageTest, AcceptsStdHeader) {
  writeConfig();
  writeFile(databaseDir / "uses_std.c", "#include <string.h>\n"
                                        "int f(void) { return 0; }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_TRUE(fs::exists(filterDir / "uses_std__f.c"));
}

TEST_F(FiltererStageTest, AcceptsNonStdHeader) {
  // Non-standard includes never disqualify a file at the pre-filter; include
  // handling is the transform step's problem.
  writeConfig();
  writeFile(databaseDir / "project.h", "int helper(void);\n");
  writeFile(databaseDir / "uses_local.c", "#include \"project.h\"\n"
                                          "int f(void) { return helper(); }\n");

  Filterer f(configPath.string());
  f.run();

  EXPECT_TRUE(fs::exists(filterDir / "uses_local__f.c"));
}

TEST_F(FiltererStageTest, HeaderSplicedSignatureDoesNotCrash) {
  // A function whose signature is spliced in from an #include'd header but
  // whose body is written directly in the main file (legal, if unusual, C -
  // the parser doesn't care about file boundaries mid-declaration).
  // FunctionDecl::getLocation() (the name token) resolves into the header, so
  // it's never registered in _allFunctions - but the body's statements are
  // genuinely in the main file, so CountingVisitor's per-node gates still let
  // them through, and getStmtParentFuncName's structural parent-walk still
  // (correctly) resolves them to "foo". Before CountingVisitor guaranteed the
  // resolved name always has a map entry, this crashed with
  // unordered_map::at and silently dropped the whole file from the run.
  writeConfig();
  writeFile(databaseDir / "signature.h", "void foo(void)\n");
  writeFile(databaseDir / "spliced.c", "#include \"signature.h\"\n"
                                       "{\n"
                                       "  int *p = 0;\n"
                                       "  (void)p;\n"
                                       "}\n"
                                       "\n"
                                       "int main(void) { foo(); return 0; }\n");

  Filterer f(configPath.string());
  f.run();

  // foo's name token resolves into the header, so SplitConsumer's own
  // isInMainFile gate (mirroring CountingVisitor's) never treats it as a
  // survivor; only main is split out.
  EXPECT_TRUE(fs::exists(filterDir / "spliced__main.c"));
}

TEST_F(FiltererStageTest, StripsFunctionFailingComplexityThreshold) {
  // ForLoops = 1,9999 requires at least one for loop per function: `plain`
  // fails and is stripped to a bare declaration; `loopy` keeps its body.
  writeConfig("ForLoops = 1,9999\n");
  writeFile(databaseDir / "mixed.c", "int loopy(int n) {\n"
                                     "  int s = 0;\n"
                                     "  for (int i = 0; i < n; i++) s += i;\n"
                                     "  return s;\n"
                                     "}\n"
                                     "int plain(int x) { return x + 1; }\n");

  Filterer f(configPath.string());
  f.run();

  // plain fails the threshold outright, so it never becomes a split target.
  EXPECT_FALSE(fs::exists(filterDir / "mixed__plain.c"));

  ASSERT_TRUE(fs::exists(filterDir / "mixed__loopy.c"));
  std::string out = readFile(filterDir / "mixed__loopy.c");
  EXPECT_NE(out.find("for (int i = 0; i < n; i++)"), std::string::npos) << out;
  EXPECT_NE(out.find("int plain(int x) ;"), std::string::npos) << out;
  EXPECT_EQ(out.find("return x + 1;"), std::string::npos) << out;
}

// filterDir resolving to databaseDir is a misconfiguration, not a supported
// mode - run() must refuse it loudly rather than silently declining every
// file (which would look like a clean run that happened to filter nothing).
TEST_F(FiltererStageTest, DatabaseDirEqualToFilterDirIsRejected) {
  std::ofstream cfg(configPath);
  cfg << "[File Locations]\n"
      << "databaseDir = " << databaseDir.string() << "\n"
      << "filterDir = " << databaseDir.string() << "\n"
      << "[Debug]\n"
      << "debugLevel = 0\n";
  cfg.close();
  writeFile(databaseDir / "keepme.c", "int add(int a, int b) { return a + b; }\n");

  Filterer f(configPath.string());
  EXPECT_DEATH(f.run(), "overlaps");
}

// With cleanOutput set, a filterDir that contains databaseDir would otherwise
// be wiped along with the source tree inside it.
TEST_F(FiltererStageTest, FilterDirContainingDatabaseDirIsRejectedEvenWithCleanOutput) {
  std::ofstream cfg(configPath);
  cfg << "[File Locations]\n"
      << "databaseDir = " << databaseDir.string() << "\n"
      << "filterDir = " << tmpDir.string() << "\n"
      << "[File Settings]\n"
      << "cleanOutput = true\n";
  cfg.close();
  writeFile(databaseDir / "keepme.c", "int add(int a, int b) { return a + b; }\n");

  Filterer f(configPath.string());
  EXPECT_DEATH(f.run(), "overlaps");
  EXPECT_TRUE(fs::exists(databaseDir / "keepme.c"));
  EXPECT_TRUE(fs::exists(configPath));
}

TEST_F(FiltererStageTest, CleanOutputRefusesToWipeFilterDirInsideDatabaseDir) {
  fs::path nested = databaseDir / "src";
  std::ofstream cfg(configPath);
  cfg << "[File Locations]\n"
      << "databaseDir = " << databaseDir.string() << "\n"
      << "filterDir = " << nested.string() << "\n"
      << "[File Settings]\n"
      << "cleanOutput = true\n";
  cfg.close();
  writeFile(nested / "keepme.c", "int add(int a, int b) { return a + b; }\n");

  Filterer f(configPath.string());
  EXPECT_DEATH(f.run(), "refusing to wipe");
  EXPECT_TRUE(fs::exists(nested / "keepme.c"));
}
