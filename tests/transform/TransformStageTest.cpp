// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Transformer.hpp"

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
// Stage-level tests for Transformer
// ---------------------------------------------------------------------------
//
// These exercise the full Transformer pipeline (transformFile / transformAll)
// against temporary directories, testing behaviors that the golden tests
// can't cover: file naming, empty-harness discard, etc. Benchmark
// finalization (.yml, .i, compile check) belongs to the verify stage and is
// covered in tests/verify/VerifyStageTest.cpp.
//
// Each test writes .c inputs into a temp filterDir, runs the transformer,
// and inspects the transformDir output.

class TransformStageTest : public ::testing::Test {
protected:
  fs::path tmpDir;
  fs::path filterDir;
  fs::path transformDir;
  fs::path configPath;

  void SetUp() override {
    tmpDir = fs::temp_directory_path() / ("transform_stage_test_" + std::to_string(getpid()));
    filterDir = tmpDir / "filtered";
    transformDir = tmpDir / "transformed";
    configPath = tmpDir / "test.config";
    fs::create_directories(filterDir);
    fs::create_directories(transformDir);

    std::ofstream cfg(configPath);
    cfg << "[File Locations]\n"
        << "filterDir = " << filterDir.string() << "\n"
        << "transformDir = " << transformDir.string() << "\n"
        << "[Debug]\n"
        << "debugLevel = 0\n";
  }

  void TearDown() override { fs::remove_all(tmpDir); }
};

TEST_F(TransformStageTest, FlatFileProducesTransformedSource) {
  writeFile(filterDir / "simple.c",
            "int add(int a, int b) { return a + b; }\n");

  Transformer t(configPath.string());
  int count = t.run();

  EXPECT_GE(count, 1);
  EXPECT_TRUE(fs::exists(transformDir / "simple.c"));

  std::string src = readFile(transformDir / "simple.c");
  EXPECT_NE(src.find("int main(void)"), std::string::npos);
  EXPECT_NE(src.find("add(__VERIFIER_nondet_int(), __VERIFIER_nondet_int());"),
            std::string::npos);
}

TEST_F(TransformStageTest, NestedPathFlattensWithUnderscores) {
  writeFile(filterDir / "owner" / "repo" / "src" / "util.c",
            "int square(int x) { return x * x; }\n");

  Transformer t(configPath.string());
  t.run();

  EXPECT_TRUE(fs::exists(transformDir / "owner_repo_src_util.c"));
}

TEST_F(TransformStageTest, EmptyHarnessDiscarded) {
  // Every function takes a struct by value, which has no nondet equivalent and
  // is not a pointer either → none can be harnessed → empty main. (A pointer
  // param would not do here any more: planPointer sizes those now.)
  writeFile(filterDir / "aggregates_only.c",
            "struct Point { int x; int y; };\n"
            "int total(struct Point p) {\n"
            "  return p.x + p.y;\n"
            "}\n");

  Transformer t(configPath.string());
  int count = t.run();

  EXPECT_EQ(count, 0);
  EXPECT_FALSE(fs::exists(transformDir / "aggregates_only.c"));
}

TEST_F(TransformStageTest, ArgcArgvMainProducesTransformedSource) {
  writeFile(filterDir / "withmain.c",
            "int main(int argc, char *argv[]) {\n"
            "  return argc;\n"
            "}\n");

  Transformer t(configPath.string());
  int count = t.run();

  EXPECT_GE(count, 1);
  EXPECT_TRUE(fs::exists(transformDir / "withmain.c"));

  std::string src = readFile(transformDir / "withmain.c");
  EXPECT_NE(src.find("original_main"), std::string::npos);
  EXPECT_NE(src.find("__havoc_cstring"), std::string::npos);
  EXPECT_NE(src.find("__VERIFIER_nondet_int"), std::string::npos);
  EXPECT_NE(src.find("abort"), std::string::npos);
}
