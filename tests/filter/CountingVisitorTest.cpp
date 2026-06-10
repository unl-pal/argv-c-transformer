#include "CountingVisitor.hpp"

#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Test helper
// ---------------------------------------------------------------------------
//
// CountingVisitor needs a live ASTContext, which means we have to actually
// parse some C code. clang::tooling::buildASTFromCodeWithArgs() does that
// in-process from a string — no temp files, no compiler invocation.
//
// We keep the ASTUnit alive in the struct below because the ASTContext it
// owns is referenced while the visitor populates the map. If the unit is
// destroyed before you read the map, you'd be looking at freed memory.

struct CountResult {
  std::unique_ptr<clang::ASTUnit> ast;
  std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> funcs =
      std::make_shared<std::unordered_map<std::string, CountingVisitor::attributes>>();
};

// Parse `code` as C and run CountingVisitor with the given type filter.
// Pass an empty `types` vector to count all types (the _allTypes fast-path).
static CountResult runCounter(const std::string &code, std::vector<unsigned int> types = {}) {
  CountResult r;

  // "-xc" tells clang to treat the string as a C file rather than C++.
  // Without it, buildASTFromCodeWithArgs defaults to C++ mode.
  r.ast = clang::tooling::buildASTFromCodeWithArgs(code, {"-xc"}, "test.c");
  EXPECT_NE(r.ast, nullptr) << "AST failed to build for:\n" << code;
  if (!r.ast)
    return r;

  CountingVisitor visitor(&r.ast->getASTContext(), types, r.funcs);
  visitor.TraverseTranslationUnitDecl(r.ast->getASTContext().getTranslationUnitDecl());

  return r;
}

// ---------------------------------------------------------------------------
// TEST vs TEST_F — a note for reference
//
// TEST(Suite, Name)   — standalone, no shared setup. Fine when each test
//                       can build its own CountResult in one line.
//
// TEST_F(Fixture, Name) — inherits from a class that has SetUp()/TearDown().
//                         Useful when setup is expensive or identical across
//                         many tests (e.g., all tests on the same source file).
//
// We use plain TEST() here because each test has a different C snippet.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Loop counting
// ---------------------------------------------------------------------------

TEST(CountingVisitor, ForLoopInFunction) {
  auto r = runCounter("void foo() { for(int i=0;i<10;i++){} }");
  ASSERT_TRUE(r.funcs->count("foo")) << "function 'foo' not registered";
  EXPECT_EQ(r.funcs->at("foo").ForLoops, 1);
}

TEST(CountingVisitor, MultipleForLoops) {
  auto r = runCounter(R"(
    void foo() {
      for(int i=0;i<3;i++){}
      for(int j=0;j<3;j++){}
    }
  )");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").ForLoops, 2);
}

TEST(CountingVisitor, WhileLoopInFunction) {
  auto r = runCounter("void foo() { int x=0; while(x<10){x++;} }");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").WhileLoops, 1);
}

// ---------------------------------------------------------------------------
// If-statement counting
// ---------------------------------------------------------------------------

TEST(CountingVisitor, IfStmt) {
  auto r = runCounter("void foo(int x) { if(x>0){} }");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").IfStmt, 1);
}

TEST(CountingVisitor, TypeIfStmtIntCondition) {
  // TypeIfStmt counts ifs whose condition is of a requested type.
  // We ask for int (BuiltinType::Int = 17).
  auto r = runCounter("void foo(int x) { if(x>0){} }", {clang::BuiltinType::Int});
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").IfStmt, 1);
  EXPECT_EQ(r.funcs->at("foo").TypeIfStmt, 1);
}

// ---------------------------------------------------------------------------
// Function registration
// ---------------------------------------------------------------------------

TEST(CountingVisitor, FunctionCountInProgram) {
  // The special "Program" key tracks file-scope counts.
  // Each unique function declaration increments Program.Functions.
  auto r = runCounter("void foo(){} void bar(){}");
  ASSERT_TRUE(r.funcs->count("Program"));
  EXPECT_EQ(r.funcs->at("Program").Functions, 2);
}

TEST(CountingVisitor, EachFunctionGetsItsOwnEntry) {
  auto r = runCounter("void foo(){} void bar(){}");
  EXPECT_TRUE(r.funcs->count("foo"));
  EXPECT_TRUE(r.funcs->count("bar"));
}

// ---------------------------------------------------------------------------
// Call counting
// ---------------------------------------------------------------------------

TEST(CountingVisitor, CallFuncCount) {
  auto r = runCounter(R"(
    void helper() {}
    void foo() { helper(); helper(); }
  )");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").CallFunc, 2);
}

// ---------------------------------------------------------------------------
// Variable counting (type-filtered)
// ---------------------------------------------------------------------------

TEST(CountingVisitor, TypeVariablesCountsOnlyMatchingType) {
  // Only int variables should be counted when we filter for int.
  auto r = runCounter("void foo() { int x; float y; int z; }", {clang::BuiltinType::Int});
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").TypeVariables, 2); // x and z
}

TEST(CountingVisitor, TypeVariablesAllTypesWhenFilterEmpty) {
  // Empty types vector → _allTypes = true → count every variable
  auto r = runCounter("void foo() { int x; float y; }");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").TypeVariables, 2);
}

// ---------------------------------------------------------------------------
// Per-function isolation
// ---------------------------------------------------------------------------

TEST(CountingVisitor, CountsAreIsolatedPerFunction) {
  auto r = runCounter(R"(
    void foo() { for(int i=0;i<1;i++){} }
    void bar() { int x; }
  )");
  ASSERT_TRUE(r.funcs->count("foo"));
  ASSERT_TRUE(r.funcs->count("bar"));
  EXPECT_EQ(r.funcs->at("foo").ForLoops, 1);
  EXPECT_EQ(r.funcs->at("bar").ForLoops, 0);
}

// ---------------------------------------------------------------------------
// Binary operators
// ---------------------------------------------------------------------------

TEST(CountingVisitor, ComparisonOperatorCounted) {
  auto r = runCounter("void foo(int x) { int y = x > 0; }", {clang::BuiltinType::Int});
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").TypeCompareOperation, 1);
}
