// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "ClangToolUtils.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// parseIniFile — every config both tools read goes through this one regex,
// so pin down exactly which line shapes it accepts and skips.
// ---------------------------------------------------------------------------

class ParseIniFileTest : public ::testing::Test {
protected:
  fs::path iniPath;

  void SetUp() override {
    iniPath = fs::temp_directory_path() /
              ("clang_tool_utils_test_" + std::to_string(getpid()) + ".config");
  }

  void TearDown() override { fs::remove(iniPath); }

  std::map<std::string, std::string> parse(const std::string &content) {
    std::ofstream(iniPath) << content;
    return parseIniFile(iniPath.string());
  }
};

TEST_F(ParseIniFileTest, ParsesSimpleKeyValue) {
  auto m = parse("debugLevel = 2\n");
  ASSERT_TRUE(m.count("debugLevel"));
  EXPECT_EQ(m.at("debugLevel"), "2");
}

TEST_F(ParseIniFileTest, ParsesPathValue) {
  auto m = parse("filterDir = /tmp/some-dir_1/sub.dir\n");
  ASSERT_TRUE(m.count("filterDir"));
  EXPECT_EQ(m.at("filterDir"), "/tmp/some-dir_1/sub.dir");
}

TEST_F(ParseIniFileTest, ParsesMinMaxPair) {
  auto m = parse("ForLoops = 1, 9999\n");
  ASSERT_TRUE(m.count("ForLoops"));
  EXPECT_EQ(m.at("ForLoops"), "1, 9999");
}

TEST_F(ParseIniFileTest, SkipsSectionHeadersAndComments) {
  auto m = parse("[File Locations]\n"
                 "# a comment line\n"
                 "; another comment style\n"
                 "\n"
                 "debugLevel = 1\n");
  EXPECT_EQ(m.size(), 1u);
  EXPECT_TRUE(m.count("debugLevel"));
}

TEST_F(ParseIniFileTest, LastDuplicateKeyWins) {
  auto m = parse("debugLevel = 1\ndebugLevel = 3\n");
  EXPECT_EQ(m.at("debugLevel"), "3");
}

TEST_F(ParseIniFileTest, MissingFileYieldsEmptyMap) {
  EXPECT_TRUE(parseIniFile("/nonexistent/path/nowhere.config").empty());
}

// ---------------------------------------------------------------------------
// Small string helpers
// ---------------------------------------------------------------------------

TEST(Trim, StripsSpacesAndTabs) {
  EXPECT_EQ(trim("  hello \t"), "hello");
  EXPECT_EQ(trim("hello"), "hello");
  EXPECT_EQ(trim(" \t "), "");
  EXPECT_EQ(trim(""), "");
}

TEST(ToArgv, NullTerminatedViewOverArgs) {
  std::vector<std::string> args = {"clang", "-xc", "file.c"};
  std::vector<const char *> argv = toArgv(args);
  ASSERT_EQ(argv.size(), 4u);
  EXPECT_STREQ(argv[0], "clang");
  EXPECT_STREQ(argv[2], "file.c");
  EXPECT_EQ(argv[3], nullptr);
}
