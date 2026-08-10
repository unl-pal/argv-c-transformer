// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "IncludeIndex.hpp"

#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace {

// Builds a temp tree under the gtest temp dir, torn down in the destructor,
// so each test gets an isolated filesystem sandbox.
class TempTree {
public:
  explicit TempTree(const std::string &name)
      : root(std::filesystem::temp_directory_path() / ("include-index-test-" + name)) {
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
  }
  ~TempTree() { std::filesystem::remove_all(root); }

  void writeFile(const std::filesystem::path &relPath, const std::string &content) {
    std::filesystem::path full = root / relPath;
    std::filesystem::create_directories(full.parent_path());
    std::ofstream(full) << content;
  }

  std::filesystem::path root;
};

} // namespace

TEST(IncludeIndex, ResolvesHeaderInDifferentSubtree) {
  TempTree tree("subtree");
  tree.writeFile("include/nested/mytypes.h", "typedef struct { int x; } point_t;\n");
  tree.writeFile("src/main.c", "#include \"nested/mytypes.h\"\nint use(point_t p) { return p.x; }\n");

  HeaderIndex index(tree.root);
  std::vector<std::string> dirs = collectLocalIncludeDirs(tree.root / "src/main.c", index);

  ASSERT_EQ(dirs.size(), 1u);
  EXPECT_EQ(std::filesystem::path(dirs[0]), tree.root / "include");
}

TEST(IncludeIndex, ResolvesBareBasenameSibling) {
  TempTree tree("sibling");
  tree.writeFile("src/foo.h", "int foo(void);\n");
  tree.writeFile("src/main.c", "#include \"foo.h\"\nint main(void) { return foo(); }\n");

  HeaderIndex index(tree.root);
  std::vector<std::string> dirs = collectLocalIncludeDirs(tree.root / "src/main.c", index);

  ASSERT_EQ(dirs.size(), 1u);
  EXPECT_EQ(std::filesystem::path(dirs[0]), tree.root / "src");
}

TEST(IncludeIndex, DropsCandidateWhoseSubdirDoesNotMatch) {
  TempTree tree("mismatch");
  // A same-named header sitting under an unrelated subdirectory should not
  // be treated as a match for an include spec naming a different subdir.
  tree.writeFile("vendor/other/mytypes.h", "typedef int unrelated_t;\n");
  tree.writeFile("src/main.c", "#include \"nested/mytypes.h\"\nint x;\n");

  HeaderIndex index(tree.root);
  std::vector<std::string> dirs = collectLocalIncludeDirs(tree.root / "src/main.c", index);

  EXPECT_TRUE(dirs.empty());
}

TEST(IncludeIndex, NoLocalIncludesYieldsNoDirs) {
  TempTree tree("none");
  tree.writeFile("src/main.c", "#include <stdio.h>\nint main(void) { return 0; }\n");

  HeaderIndex index(tree.root);
  std::vector<std::string> dirs = collectLocalIncludeDirs(tree.root / "src/main.c", index);

  EXPECT_TRUE(dirs.empty());
}

TEST(IncludeIndex, MissingRootLeavesIndexEmpty) {
  HeaderIndex index(std::filesystem::path("/no/such/directory/at/all"));
  EXPECT_EQ(index.find("foo.h"), nullptr);
}
