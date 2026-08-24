// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "ClangToolUtils.hpp"

#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(ToArgv, NullTerminatedViewOverArgs) {
  std::vector<std::string> args = {"clang", "-xc", "file.c"};
  std::vector<const char *> argv = toArgv(args);
  ASSERT_EQ(argv.size(), 4u);
  EXPECT_STREQ(argv[0], "clang");
  EXPECT_STREQ(argv[2], "file.c");
  EXPECT_EQ(argv[3], nullptr);
}
