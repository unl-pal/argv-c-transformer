// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "CountingVisitor.hpp"
#include "FilterFunctionsConsumer.hpp"
#include "RemoveVisitor.hpp"

#include <clang/Frontend/ASTUnit.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------
//
// These tests exercise the actual removal *decision* (FilterFunctionsConsumer)
// and the body-stripping *action* (RemoveVisitor) that CountingVisitorTest.cpp
// deliberately stops short of: that file only checks that attributes (like
// Concurrency) are counted correctly, not what the pipeline does with them.

namespace {

// Mirrors Filterer's default config: every complexity range is permissive
// ([0, 9999]) and every feature gate is Ignore, so a test only needs to
// override the one or two keys it cares about.
std::map<std::string, std::pair<int, int>> permissiveComplexityConfig() {
  return {
      {"CallFunc", {0, 9999}}, {"ForLoops", {0, 9999}}, {"IfStmt", {0, 9999}},
      {"Param", {0, 9999}},    {"WhileLoops", {0, 9999}},
  };
}

std::map<std::string, FeatureGate> permissiveFeatureConfig() {
  return {
      {"Concurrency", FeatureGate::Ignore},
      {"FloatingPoint", FeatureGate::Ignore},
  };
}

struct FilterResult {
  std::unique_ptr<clang::ASTUnit> ast;
  std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> funcs =
      std::make_shared<std::unordered_map<std::string, CountingVisitor::attributes>>();
  std::shared_ptr<std::vector<std::string>> toRemove =
      std::make_shared<std::vector<std::string>>();
};

// Parses `code`, runs CountingVisitor to populate real attribute counts, then
// runs FilterFunctionsConsumer with the given configs and returns what ended
// up in _ToRemove.
FilterResult runFilter(const std::string &code,
                       std::map<std::string, std::pair<int, int>> complexityConfig,
                       std::map<std::string, FeatureGate> featureConfig) {
  FilterResult r;
  r.ast = clang::tooling::buildASTFromCodeWithArgs(code, {"-xc"}, "test.c");
  EXPECT_NE(r.ast, nullptr) << "AST failed to build for:\n" << code;
  if (!r.ast)
    return r;

  CountingVisitor counter(&r.ast->getASTContext(), r.funcs);
  counter.TraverseTranslationUnitDecl(r.ast->getASTContext().getTranslationUnitDecl());

  FilterFunctionsConsumer filterConsumer(r.funcs, r.toRemove, &complexityConfig, &featureConfig);
  filterConsumer.FilterFunctions(r.ast->getASTContext());

  return r;
}

bool contains(const std::vector<std::string> &v, const std::string &name) {
  return std::find(v.begin(), v.end(), name) != v.end();
}

} // namespace

// ---------------------------------------------------------------------------
// main + concurrency
// ---------------------------------------------------------------------------

TEST(FilterFunctionsConsumer, MainRemovedWhenConcurrencyForbidden) {
  // pthread_create() called directly inside main (no helper function). This
  // is exactly the case that used to slip through filtering untouched.
  auto features = permissiveFeatureConfig();
  features["Concurrency"] = FeatureGate::Forbid;
  auto r = runFilter(R"(
    typedef unsigned long pthread_t;
    int pthread_create(pthread_t *t, void *attr, void *(*fn)(void *), void *arg);
    void *worker(void *arg) { return arg; }
    int main(void) {
      pthread_t t;
      pthread_create(&t, 0, worker, 0);
      return 0;
    }
  )",
                       permissiveComplexityConfig(), features);
  EXPECT_TRUE(contains(*r.toRemove, "main"));
}

TEST(FilterFunctionsConsumer, MainKeptWhenConcurrencyIgnored) {
  // Same body as above, but Concurrency=Ignore (the default) means the check
  // is off entirely; main should survive untouched.
  auto r = runFilter(R"(
    typedef unsigned long pthread_t;
    int pthread_create(pthread_t *t, void *attr, void *(*fn)(void *), void *arg);
    void *worker(void *arg) { return arg; }
    int main(void) {
      pthread_t t;
      pthread_create(&t, 0, worker, 0);
      return 0;
    }
  )",
                       permissiveComplexityConfig(), permissiveFeatureConfig());
  EXPECT_FALSE(contains(*r.toRemove, "main"));
}

// ---------------------------------------------------------------------------
// main + ordinary threshold checks
// ---------------------------------------------------------------------------

TEST(FilterFunctionsConsumer, MainRemovedByOrdinaryThresholdCheck) {
  // main is no longer blanket-exempt from the general min/max ladder; a
  // for-loop count over the configured max should remove it just like any
  // other function.
  auto complexity = permissiveComplexityConfig();
  complexity["ForLoops"] = {0, 0};
  auto r = runFilter(R"(
    int main(void) {
      for (int i = 0; i < 10; i++) {}
      return 0;
    }
  )",
                       complexity, permissiveFeatureConfig());
  EXPECT_TRUE(contains(*r.toRemove, "main"));
}

TEST(FilterFunctionsConsumer, MainKeptWhenThresholdsSatisfied) {
  auto r = runFilter(R"(
    int main(void) {
      for (int i = 0; i < 10; i++) {}
      return 0;
    }
  )",
                       permissiveComplexityConfig(), permissiveFeatureConfig());
  EXPECT_FALSE(contains(*r.toRemove, "main"));
}

// ---------------------------------------------------------------------------
// main + param-type check (the one exemption that remains)
// ---------------------------------------------------------------------------

TEST(FilterFunctionsConsumer, MainNotRemovedForUnsupportedArgvParam) {
  // char** (argv) has no __VERIFIER_nondet_* equivalent; an ordinary
  // function with this param type would be removed by the trailing
  // param-type check, but main is exempt from that specific check since
  // MainGenConsumer handles argc/argv itself.
  auto r = runFilter(R"(
    int main(int argc, char **argv) {
      return argc;
    }
  )",
                       permissiveComplexityConfig(), permissiveFeatureConfig());
  EXPECT_FALSE(contains(*r.toRemove, "main"));
}

TEST(FilterFunctionsConsumer, OrdinaryFunctionRemovedForUnsupportedParam) {
  // Sanity check that the param-type check still fires normally for
  // non-main functions, so the main exemption above is verified against a
  // real contrast rather than a check that never removes anything.
  auto r = runFilter(R"(
    int main(void) { return 0; }
    void helper(char **argv) {}
  )",
                       permissiveComplexityConfig(), permissiveFeatureConfig());
  EXPECT_TRUE(contains(*r.toRemove, "helper"));
  EXPECT_FALSE(contains(*r.toRemove, "main"));
}

// ---------------------------------------------------------------------------
// RemoveVisitor: does main's body actually get stripped once it's in
// _ToRemove? (FilterFunctionsConsumer only decides; RemoveVisitor acts.)
// ---------------------------------------------------------------------------

TEST(RemoveVisitor, StripsMainBodyWhenListed) {
  std::string code = R"(int main(void) { return 0; })";
  auto ast = clang::tooling::buildASTFromCodeWithArgs(code, {"-xc"}, "test.c");
  ASSERT_NE(ast, nullptr);

  clang::SourceManager &mgr = ast->getSourceManager();
  clang::Rewriter rewriter;
  rewriter.setSourceMgr(mgr, ast->getLangOpts());

  auto toRemove = std::make_shared<std::vector<std::string>>();
  toRemove->push_back("main");

  RemoveVisitor visitor(rewriter, toRemove);
  visitor.TraverseDecl(ast->getASTContext().getTranslationUnitDecl());

  std::string rewritten = std::string(
      rewriter.getRewriteBufferFor(mgr.getMainFileID())->begin(),
      rewriter.getRewriteBufferFor(mgr.getMainFileID())->end());
  EXPECT_NE(rewritten.find("int main(void) ;"), std::string::npos) << rewritten;
  EXPECT_EQ(rewritten.find("return 0"), std::string::npos) << rewritten;
}

TEST(RemoveVisitor, LeavesMainBodyWhenNotListed) {
  std::string code = R"(int main(void) { return 0; })";
  auto ast = clang::tooling::buildASTFromCodeWithArgs(code, {"-xc"}, "test.c");
  ASSERT_NE(ast, nullptr);

  clang::SourceManager &mgr = ast->getSourceManager();
  clang::Rewriter rewriter;
  rewriter.setSourceMgr(mgr, ast->getLangOpts());

  auto toRemove = std::make_shared<std::vector<std::string>>(); // empty

  RemoveVisitor visitor(rewriter, toRemove);
  visitor.TraverseDecl(ast->getASTContext().getTranslationUnitDecl());

  // No rewrite happened at all, so there's no rewrite buffer for this file.
  EXPECT_EQ(rewriter.getRewriteBufferFor(mgr.getMainFileID()), nullptr);
}
