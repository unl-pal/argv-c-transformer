#include "include/Transformer.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

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
// can't cover: file naming, yml generation, empty-harness discard, etc.
//
// Each test writes .c inputs into a temp filterDir, runs the transformer,
// and inspects the benchmarkDir output.

class TransformStageTest : public ::testing::Test {
protected:
  fs::path tmpDir;
  fs::path filterDir;
  fs::path benchmarkDir;
  fs::path configPath;

  void SetUp() override {
    tmpDir = fs::temp_directory_path() / ("transform_stage_test_" + std::to_string(getpid()));
    filterDir = tmpDir / "filtered";
    benchmarkDir = tmpDir / "benchmarks";
    configPath = tmpDir / "test.config";
    fs::create_directories(filterDir);
    fs::create_directories(benchmarkDir);

    std::ofstream cfg(configPath);
    cfg << "[File Locations]\n"
        << "filterDir = " << filterDir.string() << "\n"
        << "benchmarkDir = " << benchmarkDir.string() << "\n"
        << "[Debugging Flags]\n"
        << "debugLevel = 0\n"
        << "[File Requirements and Settings]\n"
        << "keepCompilesOnly = true\n";
  }

  void TearDown() override { fs::remove_all(tmpDir); }
};

TEST_F(TransformStageTest, FlatFileProducesYml) {
  writeFile(filterDir / "simple.c",
            "int add(int a, int b) { return a + b; }\n");

  Transformer t(configPath.string());
  int count = t.run();

  EXPECT_GE(count, 1);
  EXPECT_TRUE(fs::exists(benchmarkDir / "simple.c"));
  EXPECT_TRUE(fs::exists(benchmarkDir / "simple.yml"));

  std::string yml = readFile(benchmarkDir / "simple.yml");
  EXPECT_NE(yml.find("input_files: 'simple.i'"), std::string::npos);
  EXPECT_NE(yml.find("format_version: '2.0'"), std::string::npos);
  EXPECT_NE(yml.find("termination.prp"), std::string::npos);
  EXPECT_NE(yml.find("no-overflow.prp"), std::string::npos);
  EXPECT_NE(yml.find("language: C"), std::string::npos);
  EXPECT_NE(yml.find("data_model: LP64"), std::string::npos);
}

TEST_F(TransformStageTest, NestedPathFlattensWithUnderscores) {
  writeFile(filterDir / "owner" / "repo" / "src" / "util.c",
            "int square(int x) { return x * x; }\n");

  Transformer t(configPath.string());
  t.run();

  EXPECT_TRUE(fs::exists(benchmarkDir / "owner_repo_src_util.c"));
  EXPECT_TRUE(fs::exists(benchmarkDir / "owner_repo_src_util.yml"));

  std::string yml = readFile(benchmarkDir / "owner_repo_src_util.yml");
  EXPECT_NE(yml.find("input_files: 'owner_repo_src_util.i'"), std::string::npos);
}

TEST_F(TransformStageTest, EmptyHarnessDiscardedNoYml) {
  // All functions have pointer params → none can be harnessed → empty main
  writeFile(filterDir / "ptrs_only.c",
            "void process(int *data, int len) {\n"
            "  for (int i = 0; i < len; i++) data[i]++;\n"
            "}\n");

  Transformer t(configPath.string());
  int count = t.run();

  EXPECT_EQ(count, 0);
  EXPECT_FALSE(fs::exists(benchmarkDir / "ptrs_only.c"));
  EXPECT_FALSE(fs::exists(benchmarkDir / "ptrs_only.yml"));
}

TEST_F(TransformStageTest, KeepCompilesOnlyDiscardsUndefinedTypes) {
  // Uses an undefined type in a variable declaration — this survives
  // transformation but won't compile, so keepCompilesOnly should discard it.
  // The type is declared as a typedef so clang can parse the input, but
  // the typedef comes from a local header that gets stripped.
  writeFile(filterDir / "local_types.h",
            "typedef struct { int id; } widget_t;\n");
  writeFile(filterDir / "badtype.c",
            "#include \"local_types.h\"\n"
            "int process(widget_t w) {\n"
            "  return w.id + 1;\n"
            "}\n");

  Transformer t(configPath.string());
  int count = t.run();

  EXPECT_EQ(count, 0);
  EXPECT_FALSE(fs::exists(benchmarkDir / "badtype.c"));
  EXPECT_FALSE(fs::exists(benchmarkDir / "badtype.yml"));
}

TEST_F(TransformStageTest, KeepCompilesOffRetainsUncompilable) {
  // Same uncompilable file, but with keepCompilesOnly off — should be kept
  fs::path offConfig = tmpDir / "off.config";
  {
    std::ofstream cfg(offConfig);
    cfg << "[File Locations]\n"
        << "filterDir = " << filterDir.string() << "\n"
        << "benchmarkDir = " << benchmarkDir.string() << "\n"
        << "[Debugging Flags]\n"
        << "debugLevel = 0\n"
        << "[File Requirements and Settings]\n"
        << "keepCompilesOnly = false\n";
  }

  writeFile(filterDir / "badtype.c",
            "#include \"project.h\"\n"
            "int process(int x) {\n"
            "  return x + 1;\n"
            "}\n");

  Transformer t(offConfig.string());
  int count = t.run();

  EXPECT_GE(count, 1);
  EXPECT_TRUE(fs::exists(benchmarkDir / "badtype.c"));
}

TEST_F(TransformStageTest, ArgcArgvMainProducesBenchmark) {
  writeFile(filterDir / "withmain.c",
            "int main(int argc, char *argv[]) {\n"
            "  return argc;\n"
            "}\n");

  Transformer t(configPath.string());
  int count = t.run();

  EXPECT_GE(count, 1);
  EXPECT_TRUE(fs::exists(benchmarkDir / "withmain.c"));

  std::string src = readFile(benchmarkDir / "withmain.c");
  EXPECT_NE(src.find("original_main"), std::string::npos);
  EXPECT_NE(src.find("__havoc_cstring"), std::string::npos);
  EXPECT_NE(src.find("__VERIFIER_nondet_int"), std::string::npos);
  EXPECT_NE(src.find("abort"), std::string::npos);
}
