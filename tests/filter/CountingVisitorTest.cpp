// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

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

// Parse `code` as C and run CountingVisitor over it.
static CountResult runCounter(const std::string &code) {
  CountResult r;

  // "-xc" tells clang to treat the string as a C file rather than C++.
  // Without it, buildASTFromCodeWithArgs defaults to C++ mode.
  r.ast = clang::tooling::buildASTFromCodeWithArgs(code, {"-xc"}, "test.c");
  EXPECT_NE(r.ast, nullptr) << "AST failed to build for:\n" << code;
  if (!r.ast)
    return r;

  CountingVisitor visitor(&r.ast->getASTContext(), r.funcs);
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
  EXPECT_EQ(r.funcs->at("foo").Complexity.ForLoops, 1);
}

TEST(CountingVisitor, MultipleForLoops) {
  auto r = runCounter(R"(
    void foo() {
      for(int i=0;i<3;i++){}
      for(int j=0;j<3;j++){}
    }
  )");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").Complexity.ForLoops, 2);
}

TEST(CountingVisitor, WhileLoopInFunction) {
  auto r = runCounter("void foo() { int x=0; while(x<10){x++;} }");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").Complexity.WhileLoops, 1);
}

// ---------------------------------------------------------------------------
// If-statement counting
// ---------------------------------------------------------------------------

TEST(CountingVisitor, IfStmt) {
  auto r = runCounter("void foo(int x) { if(x>0){} }");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_EQ(r.funcs->at("foo").Complexity.IfStmt, 1);
}

// ---------------------------------------------------------------------------
// Function registration
// ---------------------------------------------------------------------------

TEST(CountingVisitor, FunctionCountInProgram) {
  // The special "Program" key tracks file-scope counts.
  // Each unique function declaration increments Program.Functions.
  auto r = runCounter("void foo(){} void bar(){}");
  ASSERT_TRUE(r.funcs->count("Program"));
  EXPECT_EQ(r.funcs->at("Program").Complexity.Functions, 2);
}

TEST(CountingVisitor, EachFunctionGetsItsOwnEntry) {
  auto r = runCounter("void foo(){} void bar(){}");
  EXPECT_TRUE(r.funcs->count("foo"));
  EXPECT_TRUE(r.funcs->count("bar"));
}

TEST(CountingVisitor, ParamCountPerFunction) {
  auto r = runCounter("void foo(int a, int b, int c){} void bar(void){}");
  ASSERT_TRUE(r.funcs->count("foo"));
  ASSERT_TRUE(r.funcs->count("bar"));
  EXPECT_EQ(r.funcs->at("foo").Complexity.Param, 3);
  EXPECT_EQ(r.funcs->at("bar").Complexity.Param, 0);
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
  EXPECT_EQ(r.funcs->at("foo").Complexity.CallFunc, 2);
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
  EXPECT_EQ(r.funcs->at("foo").Complexity.ForLoops, 1);
  EXPECT_EQ(r.funcs->at("bar").Complexity.ForLoops, 0);
}

// ---------------------------------------------------------------------------
// Concurrency detection (pthread_* types)
//
// These tests deliberately typedef pthread_mutex_t/pthread_t locally instead
// of #include <pthread.h>: detection is keyed purely on the type's spelled
// name (via StdHeaders.hpp's kStdTypeHeaders), not on which header it came
// from, so a hermetic typedef exercises the same code path as the real
// system header without depending on it being present at test time.
// ---------------------------------------------------------------------------

namespace {
const char *kPthreadStubs = R"(
  typedef struct { int dummy; } pthread_mutex_t;
  typedef unsigned long pthread_t;
  int pthread_mutex_lock(pthread_mutex_t *m);
  int pthread_mutex_unlock(pthread_mutex_t *m);
)";
}

TEST(CountingVisitor, ConcurrencyFlaggedForLocalPthreadVariable) {
  // A pthread_mutex_t declared and locked entirely inside the function should
  // flag it via the VisitVarDecl path.
  auto r = runCounter(std::string(kPthreadStubs) + R"(
    void worker() {
      pthread_mutex_t m;
      pthread_mutex_lock(&m);
      pthread_mutex_unlock(&m);
    }
  )");
  ASSERT_TRUE(r.funcs->count("worker"));
  EXPECT_TRUE(r.funcs->at("worker").Features.Concurrency);
}

TEST(CountingVisitor, ConcurrencyFlaggedByCallArgumentAlone) {
  // The pthread_mutex_t lives at file scope ("Program"), never as a VarDecl
  // inside the function — only the call argument's type can catch this, so
  // this isolates the VisitCallExpr path from the VisitVarDecl path.
  auto r = runCounter(std::string(kPthreadStubs) + R"(
    pthread_mutex_t global_lock;
    void worker() {
      pthread_mutex_lock(&global_lock);
    }
  )");
  ASSERT_TRUE(r.funcs->count("worker"));
  EXPECT_TRUE(r.funcs->at("worker").Features.Concurrency);
}

TEST(CountingVisitor, ConcurrencyFlaggedForPointerTypedLocal) {
  // A pointer-typed local (pthread_mutex_t *) with no call at all should
  // still be flagged — VisitVarDecl strips the pointer before the lookup.
  auto r = runCounter(std::string(kPthreadStubs) + R"(
    void worker(pthread_mutex_t *m) {
      pthread_mutex_t *alias = m;
    }
  )");
  ASSERT_TRUE(r.funcs->count("worker"));
  EXPECT_TRUE(r.funcs->at("worker").Features.Concurrency);
}

TEST(CountingVisitor, ConcurrencyNotSetForCleanFunction) {
  auto r = runCounter(std::string(kPthreadStubs) + R"(
    int clean(int x) {
      for (int i = 0; i < x; i++) {}
      return x;
    }
  )");
  ASSERT_TRUE(r.funcs->count("clean"));
  EXPECT_FALSE(r.funcs->at("clean").Features.Concurrency);
}

TEST(CountingVisitor, ConcurrencyIsolatedPerFunction) {
  // Only the function that actually touches a pthread type should be
  // flagged — a sibling function must not pick it up.
  auto r = runCounter(std::string(kPthreadStubs) + R"(
    void worker() {
      pthread_mutex_t m;
      pthread_mutex_lock(&m);
    }
    int clean(int x) { return x + 1; }
  )");
  ASSERT_TRUE(r.funcs->count("worker"));
  ASSERT_TRUE(r.funcs->count("clean"));
  EXPECT_TRUE(r.funcs->at("worker").Features.Concurrency);
  EXPECT_FALSE(r.funcs->at("clean").Features.Concurrency);
}

// ---------------------------------------------------------------------------
// Floating-point detection — tracked but not yet enforced by the filter.
// ---------------------------------------------------------------------------

TEST(CountingVisitor, FloatingPointFlaggedForLocalFloatVariable) {
  auto r = runCounter("void foo() { float x = 1.0f; }");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_TRUE(r.funcs->at("foo").Features.FloatingPoint);
}

TEST(CountingVisitor, FloatingPointFlaggedForDoubleParam) {
  auto r = runCounter("void foo(double x) {}");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_TRUE(r.funcs->at("foo").Features.FloatingPoint);
}

TEST(CountingVisitor, FloatingPointFlaggedForFloatReturnType) {
  auto r = runCounter("float foo(int x) { return x; }");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_TRUE(r.funcs->at("foo").Features.FloatingPoint);
}

TEST(CountingVisitor, FloatingPointNotSetForIntOnlyFunction) {
  auto r = runCounter("int foo(int x) { int y = x + 1; return y; }");
  ASSERT_TRUE(r.funcs->count("foo"));
  EXPECT_FALSE(r.funcs->at("foo").Features.FloatingPoint);
}

TEST(CountingVisitor, FloatingPointIsolatedPerFunction) {
  auto r = runCounter(R"(
    void withFloat() { float x = 1.0f; }
    int clean(int x) { return x + 1; }
  )");
  ASSERT_TRUE(r.funcs->count("withFloat"));
  ASSERT_TRUE(r.funcs->count("clean"));
  EXPECT_TRUE(r.funcs->at("withFloat").Features.FloatingPoint);
  EXPECT_FALSE(r.funcs->at("clean").Features.FloatingPoint);
}
