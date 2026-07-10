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

// Mirrors Filterer's default threshold map: every max is permissive (99999)
// and every min is permissive (0), so a test only needs to override the one
// or two keys it cares about.
std::map<std::string, int> permissiveConfig() {
  return {{"Concurrency", 0},
          {"maxCallFunc", 99999},
          {"maxForLoops", 99999},
          {"maxFunctions", 99999},
          {"maxIfStmt", 99999},
          {"maxParam", 99999},
          {"maxTypeArithmeticOperation", 99999},
          {"maxTypeCompareOperation", 99999},
          {"maxTypeIfStmt", 99999},
          {"maxTypeParameters", 99999},
          {"maxTypePostfix", 99999},
          {"maxTypePrefix", 99999},
          {"maxTypeUnaryOperation", 99999},
          {"maxTypeVariableReference", 99999},
          {"maxTypeVariables", 99999},
          {"maxWhileLoops", 99999},
          {"minCallFunc", 0},
          {"minForLoops", 0},
          {"minFunctions", 0},
          {"minIfStmt", 0},
          {"minParam", 0},
          {"minTypeArithmeticOperation", 0},
          {"minTypeCompareOperation", 0},
          {"minTypeIfStmt", 0},
          {"minTypeParameters", 0},
          {"minTypePostfix", 0},
          {"minTypePrefix", 0},
          {"minTypeUnaryOperation", 0},
          {"minTypeVariableReference", 0},
          {"minTypeVariables", 0},
          {"minWhileLoops", 0}};
}

struct FilterResult {
  std::unique_ptr<clang::ASTUnit> ast;
  std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> funcs =
      std::make_shared<std::unordered_map<std::string, CountingVisitor::attributes>>();
  std::shared_ptr<std::vector<std::string>> toRemove =
      std::make_shared<std::vector<std::string>>();
};

// Parses `code`, runs CountingVisitor to populate real attribute counts, then
// runs FilterFunctionsConsumer with `config` and returns what ended up in
// _ToRemove.
FilterResult runFilter(const std::string &code, std::map<std::string, int> config) {
  FilterResult r;
  r.ast = clang::tooling::buildASTFromCodeWithArgs(code, {"-xc"}, "test.c");
  EXPECT_NE(r.ast, nullptr) << "AST failed to build for:\n" << code;
  if (!r.ast)
    return r;

  CountingVisitor counter(&r.ast->getASTContext(), {}, r.funcs);
  counter.TraverseTranslationUnitDecl(r.ast->getASTContext().getTranslationUnitDecl());

  FilterFunctionsConsumer filterConsumer(r.funcs, r.toRemove, &config);
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

TEST(FilterFunctionsConsumer, MainRemovedWhenConcurrencyFlagged) {
  // pthread_create() called directly inside main (no helper function) — this
  // is exactly the case that used to slip through filtering untouched.
  auto config = permissiveConfig();
  config["Concurrency"] = 1;
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
                       config);
  EXPECT_TRUE(contains(*r.toRemove, "main"));
}

TEST(FilterFunctionsConsumer, MainKeptWhenConcurrencyCheckDisabled) {
  // Same body as above, but Concurrency=0 (the default) means the check is
  // off entirely — main should survive untouched.
  auto config = permissiveConfig();
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
                       config);
  EXPECT_FALSE(contains(*r.toRemove, "main"));
}

// ---------------------------------------------------------------------------
// main + ordinary threshold checks
// ---------------------------------------------------------------------------

TEST(FilterFunctionsConsumer, MainRemovedByOrdinaryThresholdCheck) {
  // main is no longer blanket-exempt from the general min/max ladder — a
  // for-loop count over the configured max should remove it just like any
  // other function.
  auto config = permissiveConfig();
  config["maxForLoops"] = 0;
  auto r = runFilter(R"(
    int main(void) {
      for (int i = 0; i < 10; i++) {}
      return 0;
    }
  )",
                       config);
  EXPECT_TRUE(contains(*r.toRemove, "main"));
}

TEST(FilterFunctionsConsumer, MainKeptWhenThresholdsSatisfied) {
  auto config = permissiveConfig();
  auto r = runFilter(R"(
    int main(void) {
      for (int i = 0; i < 10; i++) {}
      return 0;
    }
  )",
                       config);
  EXPECT_FALSE(contains(*r.toRemove, "main"));
}

// ---------------------------------------------------------------------------
// main + param-type check (the one exemption that remains)
// ---------------------------------------------------------------------------

TEST(FilterFunctionsConsumer, MainNotRemovedForUnsupportedArgvParam) {
  // char** (argv) has no __VERIFIER_nondet_* equivalent — an ordinary
  // function with this param type would be removed by the trailing
  // param-type check, but main is exempt from that specific check since
  // MainGenConsumer handles argc/argv itself.
  auto config = permissiveConfig();
  auto r = runFilter(R"(
    int main(int argc, char **argv) {
      return argc;
    }
  )",
                       config);
  EXPECT_FALSE(contains(*r.toRemove, "main"));
}

TEST(FilterFunctionsConsumer, OrdinaryFunctionRemovedForUnsupportedParam) {
  // Sanity check that the param-type check still fires normally for
  // non-main functions, so the main exemption above is verified against a
  // real contrast rather than a check that never removes anything.
  auto config = permissiveConfig();
  auto r = runFilter(R"(
    int main(void) { return 0; }
    void helper(char **argv) {}
  )",
                       config);
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
