// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Transformer.hpp"
#include "include/Verifier.hpp"

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
// Stage-level tests for Verifier
// ---------------------------------------------------------------------------
//
// These run the real transform → verify chain against temporary directories:
// filtered-style .c inputs go through Transformer into transformDir, then
// Verifier re-checks/repairs them into benchmarkDir. Covered here: the
// post-transform metric re-check (strip + unharness of degraded functions,
// empty-harness discard), and benchmark finalization (.yml, .i,
// keepCompilesOnly) that used to live in the transform stage.

class VerifyStageTest : public ::testing::Test {
protected:
  fs::path tmpDir;
  fs::path filterDir;
  fs::path transformDir;
  fs::path benchmarkDir;
  fs::path configPath;

  void SetUp() override {
    tmpDir = fs::temp_directory_path() / ("verify_stage_test_" + std::to_string(getpid()));
    filterDir = tmpDir / "filtered";
    transformDir = tmpDir / "transformed";
    benchmarkDir = tmpDir / "benchmarks";
    configPath = tmpDir / "test.config";
    fs::create_directories(filterDir);
    fs::create_directories(transformDir);
    fs::create_directories(benchmarkDir);
    writeConfig();
  }

  void TearDown() override { fs::remove_all(tmpDir); }

  // Writes the config with common path settings plus any extra key lines.
  void writeConfig(const std::string &extra = "") {
    std::ofstream cfg(configPath);
    cfg << "[File Locations]\n"
        << "filterDir = " << filterDir.string() << "\n"
        << "transformDir = " << transformDir.string() << "\n"
        << "benchmarkDir = " << benchmarkDir.string() << "\n"
        << "[Debug]\n"
        << "debugLevel = 0\n"
        << extra;
  }

  // Runs the transform → verify chain over whatever is in filterDir.
  int transformAndVerify() {
    Transformer t(configPath.string());
    t.run();
    Verifier v(configPath.string());
    return v.run();
  }
};

TEST_F(VerifyStageTest, FlatFileProducesYml) {
  // add() has an additive binary op (no-overflow-eligible) but no loop, so
  // this also pins the general .yml shape without overlapping the dedicated
  // property-selection tests below.
  writeFile(filterDir / "simple.c",
            "int add(int a, int b) { return a + b; }\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  EXPECT_TRUE(fs::exists(benchmarkDir / "simple.c"));
  EXPECT_TRUE(fs::exists(benchmarkDir / "simple.i"));
  EXPECT_TRUE(fs::exists(benchmarkDir / "simple.yml"));

  // Verifier::writeHarnessHeader copies the shared harness header into
  // benchmarkDir once per run, and the #include for it must not survive into
  // the preprocessed .i (preprocessing inlines it - the .i is what a
  // verifier actually reads, so it must not depend on the header file
  // existing on disk). reach_error()'s unconditional assert(0) does leave the
  // header's path behind as an inert __FILE__ string in __assert_fail's
  // arguments - that's fine, it's just a diagnostic string, not a real
  // dependency.
  EXPECT_TRUE(fs::exists(benchmarkDir / "argv_c_harness.h"));
  EXPECT_EQ(readFile(benchmarkDir / "simple.i").find("#include"), std::string::npos);

  std::string yml = readFile(benchmarkDir / "simple.yml");
  EXPECT_NE(yml.find("input_files: 'simple.i'"), std::string::npos);
  EXPECT_NE(yml.find("format_version: '2.0'"), std::string::npos);
  EXPECT_NE(yml.find("no-overflow.prp"), std::string::npos);
  EXPECT_EQ(yml.find("termination.prp"), std::string::npos);
  EXPECT_NE(yml.find("language: C"), std::string::npos);
  EXPECT_NE(yml.find("data_model: LP64"), std::string::npos);
}

TEST_F(VerifyStageTest, HeaderDefinedStructIsOpaqueAndStillCompiles) {
  // The main-file constraint, end to end. IncludeFinder strips the quoted
  // #include as a *textual* edit after preprocessing has already run, so the
  // AST still holds a complete definition of struct Rect while the output
  // file will not declare it at all. Sizing the block with sizeof(struct Rect)
  // would parse fine here and then fail to compile as a benchmark, which is
  // why planPointer tests isInMainFile rather than isCompleteType.
  writeFile(filterDir / "shapes.h", "struct Rect { int w; int h; };\n");
  writeFile(filterDir / "area.c", "#include \"shapes.h\"\n"
                                  "int tag(struct Rect *r) { return r != 0; }\n");

  int count = transformAndVerify();

  ASSERT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "area.c"));
  std::string out = readFile(benchmarkDir / "area.c");

  // The include is gone, so the definition is gone with it.
  EXPECT_EQ(out.find("#include \"shapes.h\""), std::string::npos);
  EXPECT_EQ(out.find("struct Rect {"), std::string::npos);
  // Therefore the block must be the flat byte count, never sizeof.
  EXPECT_EQ(out.find("sizeof(struct Rect)"), std::string::npos);
  EXPECT_NE(out.find("__HAVOC_BLOCK_MAX"), std::string::npos);
  // A produced benchmark means checkCompilable passed under keepCompilesOnly.
  EXPECT_TRUE(fs::exists(benchmarkDir / "area.i"));
}

TEST_F(VerifyStageTest, HeaderTypedefStructIsForwardDeclaredAndStillCompiles) {
  // Same constraint as above, but the pointee is spelled through a typedef.
  // Hoisting the struct tag is not enough here: the anonymous struct has no
  // tag, and the name the harness casts to is the typedef, which vanished with
  // the include. pointeeFwdDecl must re-declare the typedef itself against a
  // synthesized tag.
  writeFile(filterDir / "types.h", "typedef struct { int lo; int hi; } Range;\n");
  writeFile(filterDir / "span.c", "#include \"types.h\"\n"
                                  "int nonEmpty(Range *r) { return r != 0; }\n");

  int count = transformAndVerify();

  ASSERT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "span.c"));
  std::string out = readFile(benchmarkDir / "span.c");

  EXPECT_EQ(out.find("#include \"types.h\""), std::string::npos);
  // The typedef name is re-declared, so the cast below has something to name.
  EXPECT_NE(out.find("typedef struct __havoc_Range Range;"), std::string::npos)
      << "missing typedef forward declaration; output was:\n"
      << out;
  // Opaque: an aligned byte buffer, nondet-filled on the stack, cast to Range*
  // at the call. No sizeof(Range) — the type has no definition in the output.
  EXPECT_NE(out.find("(Range *)__h"), std::string::npos);
  EXPECT_NE(out.find("unsigned char __h"), std::string::npos);
  EXPECT_EQ(out.find("sizeof(Range)"), std::string::npos);
  // A produced benchmark means checkCompilable passed under keepCompilesOnly:
  // without the typedef the cast is an unknown type name and this file is gone.
  EXPECT_TRUE(fs::exists(benchmarkDir / "span.i"));
}

TEST_F(VerifyStageTest, HeaderEnumPointerIsForwardDeclaredAndStillCompiles) {
  // A complete enum defined only in a stripped header: the main-file test must
  // treat it like any other tag, dropping it to Opaque rather than sizing a
  // bare int-like Block that names an enum the output no longer declares. The
  // enum tag is then hoisted so the cast compiles.
  writeFile(filterDir / "palette.h", "enum Color { RED, GREEN, BLUE };\n");
  writeFile(filterDir / "pick.c", "#include \"palette.h\"\n"
                                  "int is_set(enum Color *c) { return c != 0; }\n");

  int count = transformAndVerify();

  ASSERT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "pick.c"));
  std::string out = readFile(benchmarkDir / "pick.c");

  EXPECT_EQ(out.find("#include \"palette.h\""), std::string::npos);
  // The enum tag is hoisted, so the opaque cast to enum Color* has something to
  // name; the storage is the aligned byte buffer, never sized as an enum array.
  EXPECT_NE(out.find("enum Color;"), std::string::npos) << "missing enum forward declaration:\n"
                                                        << out;
  EXPECT_NE(out.find("(enum Color *)__h"), std::string::npos);
  EXPECT_NE(out.find("unsigned char __h"), std::string::npos);
  // A produced benchmark means checkCompilable passed under keepCompilesOnly.
  EXPECT_TRUE(fs::exists(benchmarkDir / "pick.i"));
}

TEST_F(VerifyStageTest, MainFileTypedefIsNotRedeclared) {
  // The mirror case: a typedef defined in the .c itself survives into the
  // output on its own. Re-declaring it against a synthesized tag would name a
  // second, incompatible type, so pointeeFwdDecl must stay quiet — and because
  // the definition is in the main file, the block is sized with sizeof.
  writeFile(filterDir / "local.c", "typedef struct { int lo; int hi; } Range;\n"
                                   "int nonEmpty(Range *r) { return r->hi > r->lo; }\n");

  int count = transformAndVerify();

  ASSERT_GE(count, 1);
  std::string out = readFile(benchmarkDir / "local.c");

  EXPECT_EQ(out.find("__havoc_Range"), std::string::npos)
      << "synthesized tag leaked for a main-file typedef; output was:\n"
      << out;
  // Sized with the real type: the harness declares typed Range storage, not the
  // opaque `unsigned char[...]` byte buffer a header-only type would get. (The
  // __HAVOC_BLOCK_MAX *macro* is always defined alongside the other bounds,
  // so its presence is not the signal — the buffer declaration is.)
  EXPECT_NE(out.find("Range __h"), std::string::npos);
  EXPECT_EQ(out.find("unsigned char __h"), std::string::npos);
  EXPECT_TRUE(fs::exists(benchmarkDir / "local.i"));
}

TEST_F(VerifyStageTest, PreprocessStripsFloatNNTypedefs) {
  // <stdio.h> transitively pulls in glibc's bits/floatn-common.h, whose
  // fallback typedefs for the C23 extended float types CBMC treats as
  // reserved builtin names and aborts on. No generated code ever spells
  // _Float32 etc., so Verifier::preprocess must strip those typedef lines
  // from the .i without touching anything else.
  writeFile(filterDir / "prints.c", "#include <stdio.h>\n"
                                     "int identity(int x) { return x; }\n");

  int count = transformAndVerify();

  ASSERT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "prints.i"));
  std::string i = readFile(benchmarkDir / "prints.i");
  EXPECT_EQ(i.find("_Float"), std::string::npos);
}

// ---------------------------------------------------------------------------
// selectProperties: which .prp files get attached, based on the fresh
// per-function counts taken after transform (see CountingVisitor::Complexity).
// ---------------------------------------------------------------------------

TEST_F(VerifyStageTest, LoopOnlySourceGetsTerminationNotOverflow) {
  // A pure loop with no arithmetic operators: increments (i++) come from
  // UnaryOperator, but the loop guard/body here does no +,-,*,<<,>> at all,
  // so Operations should stay at 0 and no-overflow.prp should not appear.
  writeFile(filterDir / "loopy.c",
            "void spin(int n) {\n"
            "  int i = 0;\n"
            "  while (i != n) {\n"
            "    i = n;\n"
            "  }\n"
            "}\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "loopy.yml"));
  std::string yml = readFile(benchmarkDir / "loopy.yml");
  EXPECT_NE(yml.find("termination.prp"), std::string::npos);
  EXPECT_EQ(yml.find("no-overflow.prp"), std::string::npos);
}

TEST_F(VerifyStageTest, ArithmeticOnlySourceGetsOverflowNotTermination) {
  // No loop anywhere, but a multiplicative op gives Operations > 0.
  writeFile(filterDir / "mul.c", "int scale(int a, int b) { return a * b; }\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "mul.yml"));
  std::string yml = readFile(benchmarkDir / "mul.yml");
  EXPECT_NE(yml.find("no-overflow.prp"), std::string::npos);
  EXPECT_EQ(yml.find("termination.prp"), std::string::npos);
}

TEST_F(VerifyStageTest, PlainSourceGetsNoProperties) {
  // No loop, no binary/unary arithmetic operator anywhere: neither property
  // should be selected, and the benchmark should still be produced.
  writeFile(filterDir / "flat.c", "int identity(int a) { return a; }\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "flat.yml"));
  std::string yml = readFile(benchmarkDir / "flat.yml");
  EXPECT_EQ(yml.find("termination.prp"), std::string::npos);
  EXPECT_EQ(yml.find("no-overflow.prp"), std::string::npos);
  EXPECT_NE(yml.find("properties: []\n"), std::string::npos);
}

TEST_F(VerifyStageTest, LoopAndArithmeticAcrossFunctionsGetsBoth) {
  // The two signals live in different functions; selectProperties scans the
  // whole per-file counts map, so both should be picked up regardless of
  // which function iteration order visits first. spin's loop must have an
  // observable side effect (mutating the parameter n, not just a loop-local
  // var), or HavocCallsVisitor's no-op pruning drops the whole loop/function.
  writeFile(filterDir / "both.c",
            "int spin(int n) {\n"
            "  for (int i = 0; i < n; i++) {\n"
            "    n += i;\n"
            "  }\n"
            "  return n;\n"
            "}\n"
            "int scale(int a, int b) { return a * b; }\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "both.yml"));
  std::string yml = readFile(benchmarkDir / "both.yml");
  EXPECT_NE(yml.find("termination.prp"), std::string::npos);
  EXPECT_NE(yml.find("no-overflow.prp"), std::string::npos);
}

TEST_F(VerifyStageTest, PointerDerefSourceGetsMemsafetyProperty) {
  // arr[i] is an ArraySubscriptExpr, which CountingVisitor flags as
  // PointerDeref. Keep the array local rather than a pointer param: params
  // are only harnessed when every one has a primitive nondet suffix
  // (MainGenConsumer::verifierSuffixForType), so a pointer param currently
  // leaves the function unharnessed and the file discarded (no calls at all).
  writeFile(filterDir / "deref.c",
            "void access(void) {\n"
            "  int arr[4];\n"
            "  int i = 0;\n"
            "  arr[i] = 1;\n"
            "}\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "deref.yml"));
  std::string yml = readFile(benchmarkDir / "deref.yml");
  EXPECT_NE(yml.find("valid-memsafety.prp"), std::string::npos);
}

TEST_F(VerifyStageTest, MallocFreeSourceGetsMemsafetyProperty) {
  // malloc/free are MemAlloc/MemFree signals; either alone should select
  // valid-memsafety.prp (it bundles deref/free/memtrack CHECKs in one file).
  writeFile(filterDir / "alloc.c",
            "#include <stdlib.h>\n"
            "void make_and_drop(int n) {\n"
            "  int *buf = malloc(n * sizeof(int));\n"
            "  free(buf);\n"
            "}\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "alloc.yml"));
  std::string yml = readFile(benchmarkDir / "alloc.yml");
  EXPECT_NE(yml.find("valid-memsafety.prp"), std::string::npos);
}

TEST_F(VerifyStageTest, DegradedFunctionIsStrippedAndUnharnessed) {
  // worker's only loop exists to drive the void call to helper; havocking
  // drops that call and prunes the emptied loop, so worker falls below a
  // ForLoops >= 1 threshold it met before the transform. helper keeps its
  // loop. The verify stage must strip worker, drop its call from the
  // generated main, and still produce a benchmark around helper.
  writeConfig("[Complexity Requirements]\nForLoops = 1,9999\n");
  writeFile(filterDir / "degraded.c",
            "void helper(int x) {\n"
            "  for (int i = 0; i < x; i++) x += i;\n"
            "}\n"
            "int worker(int n) {\n"
            "  for (int i = 0; i < n; i++) helper(i);\n"
            "  return n;\n"
            "}\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "degraded.c"));

  std::string src = readFile(benchmarkDir / "degraded.c");
  // helper is still harnessed; worker's harness call and body are gone.
  EXPECT_NE(src.find("helper(__VERIFIER_nondet_int());"), std::string::npos);
  EXPECT_EQ(src.find("worker(__VERIFIER_nondet_int());"), std::string::npos);
  EXPECT_EQ(src.find("return n;"), std::string::npos);
}

TEST_F(VerifyStageTest, HarnessEmptyAfterRepairIsDiscarded) {
  // Same input, but a threshold no function meets post-transform: helper has
  // one loop (needs two), worker has none once its loop is pruned. Every
  // harness call is repaired away, so no benchmark must be produced.
  writeConfig("[Complexity Requirements]\nForLoops = 2,9999\n");
  writeFile(filterDir / "degraded.c",
            "void helper(int x) {\n"
            "  for (int i = 0; i < x; i++) x += i;\n"
            "}\n"
            "int worker(int n) {\n"
            "  for (int i = 0; i < n; i++) helper(i);\n"
            "  return n;\n"
            "}\n");

  int count = transformAndVerify();

  EXPECT_EQ(count, 0);
  EXPECT_FALSE(fs::exists(benchmarkDir / "degraded.c"));
  EXPECT_FALSE(fs::exists(benchmarkDir / "degraded.yml"));
}

TEST_F(VerifyStageTest, KeepCompilesOnlyDiscardsUndefinedTypes) {
  // Uses a type from a local header that gets stripped during transform -
  // the transformed file parses but won't compile, so keepCompilesOnly
  // (default true) should discard it in the verify stage.
  writeFile(filterDir / "local_types.h",
            "typedef struct { int id; } widget_t;\n");
  writeFile(filterDir / "badtype.c",
            "#include \"local_types.h\"\n"
            "int process(widget_t w) {\n"
            "  return w.id + 1;\n"
            "}\n");

  int count = transformAndVerify();

  EXPECT_EQ(count, 0);
  EXPECT_FALSE(fs::exists(benchmarkDir / "badtype.c"));
  EXPECT_FALSE(fs::exists(benchmarkDir / "badtype.yml"));
}

TEST_F(VerifyStageTest, AssertRewriteAddsUnreachCallProperty) {
  // assert(cond) is rewritten to reach_error() by the transform stage;
  // reach_error must be exempt from the post-transform threshold re-check
  // (isVerifierGenerated) or its trivial body would get stripped, and its
  // presence in the reparsed counts should add unreach-call.prp.
  writeFile(filterDir / "checked.c",
            "#include <assert.h>\n"
            "int add(int a, int b) {\n"
            "  int r = a + b;\n"
            "  assert(r >= a);\n"
            "  return r;\n"
            "}\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "checked.c"));
  ASSERT_TRUE(fs::exists(benchmarkDir / "checked.yml"));

  std::string src = readFile(benchmarkDir / "checked.c");
  // reach_error() is defined unconditionally in argv_c_harness.h now, not
  // per-file; only the rewritten call site and the runtime include remain
  // in the .c itself.
  EXPECT_NE(src.find("if (!(r >= a)) reach_error();"), std::string::npos);
  EXPECT_NE(src.find("#include \"argv_c_harness.h\""), std::string::npos);

  std::string yml = readFile(benchmarkDir / "checked.yml");
  EXPECT_NE(yml.find("unreach-call.prp"), std::string::npos);
}

TEST_F(VerifyStageTest, ArgcArgvMainSurvivesVerify) {
  // original_main takes (int, char**): the verify re-check must not trip
  // over its unsupported params (no param check post-transform) and the
  // synthesized argv harness in main must be left alone.
  writeFile(filterDir / "withmain.c",
            "int main(int argc, char *argv[]) {\n"
            "  return argc;\n"
            "}\n");

  int count = transformAndVerify();

  EXPECT_GE(count, 1);
  ASSERT_TRUE(fs::exists(benchmarkDir / "withmain.c"));

  std::string src = readFile(benchmarkDir / "withmain.c");
  EXPECT_NE(src.find("original_main(argc, __havoc_argv_fill(argc));"), std::string::npos);
}
