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
  /// Stand-in for the original repo tree: unlike filterDir it keeps headers.
  fs::path dbDir;
  /// Same as configPath but with databaseDir set, for local-header resolution.
  fs::path dbConfigPath;

  void SetUp() override {
    tmpDir = fs::temp_directory_path() / ("transform_stage_test_" + std::to_string(getpid()));
    filterDir = tmpDir / "filtered";
    transformDir = tmpDir / "transformed";
    dbDir = tmpDir / "repo";
    configPath = tmpDir / "test.config";
    dbConfigPath = tmpDir / "test_db.config";
    fs::create_directories(filterDir);
    fs::create_directories(transformDir);
    fs::create_directories(dbDir);

    std::ofstream cfg(configPath);
    cfg << "[File Locations]\n"
        << "filterDir = " << filterDir.string() << "\n"
        << "transformDir = " << transformDir.string() << "\n"
        << "[Debug]\n"
        << "debugLevel = 0\n";

    std::ofstream dbCfg(dbConfigPath);
    dbCfg << "[File Locations]\n"
          << "databaseDir = " << dbDir.string() << "\n"
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

// Regression: Transformer::parseConfig used to drop the config's databaseDir,
// so a standalone `transform` run built an empty HeaderIndex and passed no -I
// paths. The quoted #include then failed to resolve and clang error-recovered
// by treating the unknown `Range *` as `int *` — silently producing a harness
// that allocates a wrongly-sized block cast to a type the output never
// declares. argv-c masked this by calling setDatabaseDir() explicitly.
TEST_F(TransformStageTest, ConfigDatabaseDirResolvesLocalHeaders) {
  // The original repo tree keeps the header; the filtered tree mirrors only .c
  // files, so `mytypes.h` is reachable *only* via databaseDir.
  writeFile(dbDir / "include" / "mytypes.h",
            "typedef struct { int lo; int hi; } Range;\n"
            "int rangeWidth(Range *r);\n");
  const char *src = "#include \"mytypes.h\"\n"
                    "int rangeWidth(Range *r) { return r->hi - r->lo; }\n"
                    "int span(Range *r, int n) { return rangeWidth(r) + n; }\n";
  writeFile(dbDir / "src" / "work.c", src);
  writeFile(filterDir / "src" / "work.c", src);

  Transformer t(dbConfigPath.string());
  ASSERT_GE(t.run(), 1);

  std::string out = readFile(transformDir / "src_work.c");

  // `Range` resolved to a real RecordDecl whose definition is NOT in the main
  // file (the #include is stripped textually), so planPointer returns Opaque.
  // This pins the exact emission on purpose: if the Opaque policy changes,
  // update this string to match the new intended output.
  EXPECT_NE(out.find("span((Range *)__havoc_block(__HAVOC_OPAQUE_BYTES)"), std::string::npos)
      << "harness did not emit the Opaque plan for Range *; output was:\n"
      << out;

  // The specific pre-fix symptom: with no -I path clang error-recovered by
  // substituting `int` for the unknown `Range`, and planPointer sized the
  // bogus `int *` as a Block. Survives any change to the Opaque policy above.
  EXPECT_EQ(out.find("(int *)__havoc_block"), std::string::npos)
      << "unknown-typename error recovery leaked into the harness";
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
