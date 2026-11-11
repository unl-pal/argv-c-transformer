// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "ClangToolUtils.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

TEST(ToArgv, NullTerminatedViewOverArgs) {
  std::vector<std::string> args = {"clang", "-xc", "file.c"};
  std::vector<const char *> argv = toArgv(args);
  ASSERT_EQ(argv.size(), 4u);
  EXPECT_STREQ(argv[0], "clang");
  EXPECT_STREQ(argv[2], "file.c");
  EXPECT_EQ(argv[3], nullptr);
}

// configureSysroot mutates process-global state and reads $ARGV_SYSROOT, so
// each test starts from a cleared sysroot and env and restores both after.
class SysrootTest : public ::testing::Test {
protected:
  fs::path dir;
  std::optional<std::string> savedEnv;

  void SetUp() override {
    dir = fs::temp_directory_path() / ("sysroot_test_" + std::to_string(getpid()));
    fs::create_directories(dir / "usr" / "include");
    if (const char *env = std::getenv("ARGV_SYSROOT")) savedEnv = env;
    unsetenv("ARGV_SYSROOT");
    globalSysroot().clear();
  }

  void TearDown() override {
    globalSysroot().clear();
    if (savedEnv)
      setenv("ARGV_SYSROOT", savedEnv->c_str(), 1);
    else
      unsetenv("ARGV_SYSROOT");
    fs::remove_all(dir);
  }
};

TEST_F(SysrootTest, ConfiguredSysrootBecomesDriverFlag) {
  configureSysroot(dir.string());
  EXPECT_EQ(sysrootArgs(), (std::vector<std::string>{"--sysroot=" + dir.string(),
                                                      "--target=x86_64-linux-gnu"}));
}

TEST(ResourceDir, FoundWithoutClangOnPath) {
  std::optional<std::string> built = buildResourceDir();
  if (!built) GTEST_SKIP() << "build LLVM's resource dir not present on this machine";
  EXPECT_EQ(fs::path(*built).filename().string(), std::to_string(CLANG_VERSION_MAJOR));

  const char *oldPath = std::getenv("PATH");
  const char *oldRes = std::getenv("CLANG_RESOURCES");
  std::string savedPath = oldPath ? oldPath : "", savedRes = oldRes ? oldRes : "";
  setenv("PATH", "/nonexistent", 1);
  unsetenv("CLANG_RESOURCES");
  std::optional<std::string> found = detectResourceDir();
  setenv("PATH", savedPath.c_str(), 1);
  if (oldRes) setenv("CLANG_RESOURCES", savedRes.c_str(), 1);

  EXPECT_EQ(found, built);
}

TEST(HostTargetWarning, OnlyNonX86HostWithoutSysrootWarns) {
  EXPECT_TRUE(hostTargetWarning(false, false).has_value());
  EXPECT_FALSE(hostTargetWarning(false, true).has_value());
  EXPECT_FALSE(hostTargetWarning(true, false).has_value());
  EXPECT_FALSE(hostTargetWarning(true, true).has_value());
}

TEST_F(SysrootTest, BuildClangArgsCarriesSysroot) {
  configureSysroot(dir.string());
  std::vector<std::string> args = buildClangArgs("file.c", "/res");
  EXPECT_NE(std::find(args.begin(), args.end(), "--sysroot=" + dir.string()), args.end());
}

TEST_F(SysrootTest, EnvVarIsFallbackForConfig) {
  setenv("ARGV_SYSROOT", dir.string().c_str(), 1);
  configureSysroot("");
  EXPECT_EQ(globalSysroot(), dir.string());
}

TEST_F(SysrootTest, ConfigWinsOverEnvVar) {
  fs::path other = dir / "other";
  fs::create_directories(other);
  setenv("ARGV_SYSROOT", dir.string().c_str(), 1);
  configureSysroot(other.string());
  EXPECT_EQ(globalSysroot(), other.string());
}

TEST_F(SysrootTest, RelativeSysrootIsMadeAbsolute) {
  fs::path saved = fs::current_path();
  fs::current_path(dir);
  configureSysroot("usr");
  fs::current_path(saved);
  EXPECT_EQ(globalSysroot(), (dir / "usr").string());
}

TEST_F(SysrootTest, UnsetClearsPreviousSysroot) {
  configureSysroot(dir.string());
  configureSysroot("");
  EXPECT_TRUE(globalSysroot().empty());
#ifndef __APPLE__
  EXPECT_TRUE(sysrootArgs().empty()); // host headers, exactly as with no sysroot support
#endif
}

TEST_F(SysrootTest, MissingSysrootExits) {
  EXPECT_EXIT(configureSysroot((dir / "nope").string()), ::testing::ExitedWithCode(1),
              "not a directory");
}
