// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "Filterer.hpp"
#include "include/Transformer.hpp"
#include "include/Verifier.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// End-to-end tests for declaration + macro closure over project-local headers.
// ---------------------------------------------------------------------------
//
// These run the whole filter → transform → verify chain, unlike VerifyStageTest
// which starts at transform: closure runs in the *filter* stage, so a test that
// skips filter would exercise none of it.
//
// The verify stage compile-checks under keepCompilesOnly, so a produced
// benchmark is itself the strongest assertion here — the .c has to stand alone
// with every local header gone. The string checks pin down *how* it stands
// alone, so a regression that merely re-broke the mechanism differently would
// still be caught.

class HeaderClosureTest : public ::testing::Test {
protected:
  fs::path tmpDir;
  fs::path repoDir; // the "database": .c files and their local headers
  fs::path filterDir;
  fs::path transformDir;
  fs::path benchmarkDir;
  fs::path configPath;

  void SetUp() override {
    tmpDir = fs::temp_directory_path() / ("header_closure_test_" + std::to_string(getpid()));
    repoDir = tmpDir / "repo";
    filterDir = tmpDir / "filtered";
    transformDir = tmpDir / "transformed";
    benchmarkDir = tmpDir / "benchmarks";
    configPath = tmpDir / "test.config";
    fs::create_directories(repoDir);

    std::ofstream cfg(configPath);
    cfg << "[File Locations]\n"
        << "databaseDir = " << repoDir.string() << "\n"
        << "filterDir = " << filterDir.string() << "\n"
        << "transformDir = " << transformDir.string() << "\n"
        << "benchmarkDir = " << benchmarkDir.string() << "\n"
        << "[Debug]\n"
        << "debugLevel = 0\n";
  }

  void TearDown() override { fs::remove_all(tmpDir); }

  void writeRepoFile(const std::string &name, const std::string &content) {
    fs::path path = repoDir / name;
    fs::create_directories(path.parent_path());
    std::ofstream(path) << content;
  }

  static std::string readFile(const fs::path &path) {
    std::ifstream in(path);
    std::stringstream buf;
    buf << in.rdbuf();
    return buf.str();
  }

  /** @brief Runs the full pipeline and returns the filter stage's output for `name`. */
  std::string runPipeline(const std::string &name) {
    Filterer f(configPath.string());
    f.run();
    Transformer t(configPath.string());
    t.setDatabaseDir(repoDir.string());
    t.run();
    Verifier v(configPath.string());
    _benchmarks = v.run();
    _filtered = readFile(filterDir / name);
    return readFile(benchmarkDir / name);
  }

  int benchmarks() const { return _benchmarks; }
  const std::string &filtered() const { return _filtered; }

private:
  int _benchmarks = 0;
  std::string _filtered;
};

// --- macro closure ---------------------------------------------------------

TEST_F(HeaderClosureTest, ObjectLikeMacroInArrayBoundIsReEmitted) {
  // The case the declaration closure structurally cannot reach: by the time an
  // AST exists, buf is an array of 64 with a macro-expansion SourceLocation and
  // no decl to walk back to. Only the preprocessor ever saw BUFSIZE.
  writeRepoFile("sizes.h", "#define BUFSIZE 64\n");
  writeRepoFile("buf.c", "#include \"sizes.h\"\n"
                         "int first(int n) {\n"
                         "  char buf[BUFSIZE];\n"
                         "  buf[0] = (char)n;\n"
                         "  return buf[0];\n"
                         "}\n");

  std::string out = runPipeline("buf.c");

  ASSERT_GE(benchmarks(), 1) << "benchmark discarded; filtered output was:\n" << filtered();
  EXPECT_NE(out.find("#define BUFSIZE 64"), std::string::npos) << out;
  EXPECT_EQ(out.find("#include \"sizes.h\""), std::string::npos);
  // Re-emitted, not expanded: the .c stays auditable.
  EXPECT_NE(out.find("char buf[BUFSIZE]"), std::string::npos) << out;
}

TEST_F(HeaderClosureTest, FunctionLikeMacroAndItsNestedMacroAreReEmitted) {
  // TRIPLE's body names SCALE, so re-emitting TRIPLE alone leaves the output
  // referencing an undefined identifier. The macro closure has to be transitive.
  writeRepoFile("math.h", "#define SCALE 3\n"
                          "#define TRIPLE(x) ((x) * SCALE)\n");
  writeRepoFile("scale.c", "#include \"math.h\"\n"
                           "int thrice(int n) { return TRIPLE(n); }\n");

  std::string out = runPipeline("scale.c");

  ASSERT_GE(benchmarks(), 1) << "benchmark discarded; filtered output was:\n" << filtered();
  EXPECT_NE(out.find("#define TRIPLE(x) ((x) * SCALE)"), std::string::npos) << out;
  EXPECT_NE(out.find("#define SCALE 3"), std::string::npos) << out;
}

TEST_F(HeaderClosureTest, MacroFromSurvivingSystemIncludeIsNotReEmitted) {
  // Negative. System headers are included *by reference* — the target machine
  // has them. Re-emitting EOF would be a redefinition of something <stdio.h>
  // still provides, and would start the slide toward inlining glibc.
  writeRepoFile("tag.h", "#define SENTINEL 7\n");
  writeRepoFile("eof.c", "#include <stdio.h>\n"
                         "#include \"tag.h\"\n"
                         "int at_end(int c) { return c == EOF ? SENTINEL : 0; }\n");

  std::string out = runPipeline("eof.c");

  ASSERT_GE(benchmarks(), 1) << "benchmark discarded; filtered output was:\n" << filtered();
  EXPECT_EQ(out.find("#define EOF"), std::string::npos) << out;
  EXPECT_NE(out.find("#include <stdio.h>"), std::string::npos) << out;
  // The local macro alongside it still comes through, so this is not just
  // asserting that nothing happened.
  EXPECT_NE(out.find("#define SENTINEL 7"), std::string::npos) << out;
}

// --- declaration closure ---------------------------------------------------

TEST_F(HeaderClosureTest, HeaderStructIsInlinedAndSizedExactly) {
  // The payoff. Before closure, a header-defined struct was havocked as a flat
  // __HAVOC_OPAQUE_BYTES block, because planPointer tests isInMainFile and the
  // definition was about to be deleted. Inlining it makes that test true, so
  // the exact layout is used with no change to HavocPolicy at all.
  writeRepoFile("point.h", "struct Point { int x; int y; };\n");
  writeRepoFile("area.c", "#include \"point.h\"\n"
                          "int area(struct Point *p) { return p->x * p->y; }\n");

  std::string out = runPipeline("area.c");

  ASSERT_GE(benchmarks(), 1) << "benchmark discarded; filtered output was:\n" << filtered();
  EXPECT_NE(out.find("struct Point { int x; int y; };"), std::string::npos) << out;
  EXPECT_NE(out.find("(struct Point *)__havoc_block(sizeof(struct Point)"), std::string::npos)
      << out;
  EXPECT_TRUE(fs::exists(benchmarkDir / "area.i"));
}

TEST_F(HeaderClosureTest, SystemIncludeReachedThroughLocalHeaderIsReEmitted) {
  // An inlined decl can depend on a system type. size_t is reachable only via
  // types.h's own #include <stddef.h>, and that directive vanishes with the
  // header — so it has to be re-emitted, or the inlined typedef names nothing.
  writeRepoFile("types.h", "#include <stddef.h>\n"
                           "typedef struct { size_t n; } Buf;\n");
  writeRepoFile("count.c", "#include \"types.h\"\n"
                           "int nonempty(Buf *b) { return b->n > 0; }\n");

  std::string out = runPipeline("count.c");

  ASSERT_GE(benchmarks(), 1) << "benchmark discarded; filtered output was:\n" << filtered();
  EXPECT_NE(out.find("#include <stddef.h>"), std::string::npos) << out;
  EXPECT_NE(out.find("size_t n;"), std::string::npos) << out;
  // Complete definition inlined, so the block is sized rather than opaque.
  EXPECT_NE(out.find("sizeof(Buf)"), std::string::npos) << out;
}

TEST_F(HeaderClosureTest, HeaderFunctionBodyIsNotInlinedButPrototypeIs) {
  // Negative, and the one deliberate exception to "full definitions
  // everywhere". The transform is intraprocedural: HavocCallsVisitor havocs
  // every call whose callee is declared in-file, so an inlined header body is
  // unreachable except through the generated harness, where it would only
  // inflate the benchmark and add a spurious harness target. A prototype gets
  // the desired behaviour with no dead code. This inverts when interprocedural
  // analysis lands.
  writeRepoFile("dbl.h", "static inline int hdr_double(int x) { return x * 2; }\n");
  writeRepoFile("call.c", "#include \"dbl.h\"\n"
                          "int twice(int n) { return hdr_double(n); }\n");

  std::string out = runPipeline("call.c");

  ASSERT_GE(benchmarks(), 1) << "benchmark discarded; filtered output was:\n" << filtered();
  EXPECT_EQ(out.find("return x * 2;"), std::string::npos) << "header body was inlined:\n" << out;
  EXPECT_NE(out.find("int hdr_double(int x);"), std::string::npos) << out;
  // The prototype is what makes the call havockable, which is the whole point.
  EXPECT_NE(out.find("__VERIFIER_nondet_int()"), std::string::npos) << out;
  // hdr_double is declared, never defined, and never called: harnessing it
  // would be a link error, so it must not appear in main.
  EXPECT_EQ(out.find("hdr_double(__VERIFIER"), std::string::npos) << out;
}

TEST_F(HeaderClosureTest, UnreferencedHeaderDeclarationIsNotInlined) {
  // Negative: the roots actually bound the closure. Inlining a whole header
  // would compile just as well, so nothing but this test distinguishes a real
  // closure from a copy-the-header shortcut.
  writeRepoFile("wide.h", "struct Used { int a; };\n"
                          "struct Unused { int b; };\n"
                          "#define UNUSED_LIMIT 99\n");
  writeRepoFile("narrow.c", "#include \"wide.h\"\n"
                            "int get(struct Used *u) { return u->a; }\n");

  std::string out = runPipeline("narrow.c");

  ASSERT_GE(benchmarks(), 1) << "benchmark discarded; filtered output was:\n" << filtered();
  EXPECT_NE(out.find("struct Used { int a; };"), std::string::npos) << out;
  EXPECT_EQ(out.find("struct Unused"), std::string::npos) << out;
  EXPECT_EQ(out.find("UNUSED_LIMIT"), std::string::npos) << out;
}

TEST_F(HeaderClosureTest, RejectedFunctionBodyIsNotARoot) {
  // Roots are surviving bodies plus all kept signatures. A function the filter
  // rejected has its body replaced with ';', so a type used only inside that
  // body is dead — but its *signature* types stay live, because RemoveVisitor
  // deliberately leaves signatures for transform to read return types from.
  std::ofstream(configPath, std::ios::app) << "IfStmt = 1\n";
  writeRepoFile("shapes.h", "struct Kept { int a; };\n"
                            "struct BodyOnly { int b; };\n");
  writeRepoFile("gate.c",
                "#include \"shapes.h\"\n"
                // No if statement: rejected, body stripped to ';'.
                "int plain(struct Kept *k) {\n"
                "  struct BodyOnly local;\n"
                "  local.b = k->a;\n"
                "  return local.b;\n"
                "}\n"
                // Has an if: survives, so it keeps the harness alive.
                "int branchy(struct Kept *k) { if (k->a > 0) return 1; return 0; }\n");

  std::string out = runPipeline("gate.c");

  ASSERT_GE(benchmarks(), 1) << "benchmark discarded; filtered output was:\n" << filtered();
  // Signature type of the rejected function: still inlined.
  EXPECT_NE(out.find("struct Kept { int a; };"), std::string::npos) << out;
  // Reached only from the stripped body: not a root, so not inlined.
  EXPECT_EQ(out.find("struct BodyOnly"), std::string::npos) << out;
}
