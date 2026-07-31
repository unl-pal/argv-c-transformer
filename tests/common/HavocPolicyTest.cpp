// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "HavocPolicy.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

// ---------------------------------------------------------------------------
// planPointer is pure AST -> decision, so these tests need no Rewriter and no
// frontend action: parse a declaration, hand the classifier the parameter's
// type, and inspect the plan. Note getOriginalType() rather than getType():
// the pre-decay type is what preserves `T[N]`'s declared bound, and it is what
// both production call sites use.
// ---------------------------------------------------------------------------

namespace {

struct Parsed {
  std::unique_ptr<clang::ASTUnit> ast;
  PointerPlan plan;
};

// Parses `code`, finds the first parameter of the function named `func`, and
// classifies its declared (pre-decay) type. The ASTUnit is returned alongside
// the plan because the plan's strings outlive nothing but the AST must stay
// alive for the duration of the parse itself.
Parsed planFirstParam(const std::string &code, const std::string &func = "f") {
  Parsed p;
  p.ast = clang::tooling::buildASTFromCodeWithArgs(code, {"-xc"}, "test.c");
  EXPECT_NE(p.ast, nullptr) << "AST failed to build for:\n" << code;
  if (!p.ast)
    return p;

  clang::ASTContext &ctx = p.ast->getASTContext();
  for (clang::Decl *decl : ctx.getTranslationUnitDecl()->decls()) {
    auto *fn = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (!fn || fn->getNameAsString() != func || fn->param_empty())
      continue;
    p.plan = planPointer(fn->getParamDecl(0)->getOriginalType(), ctx.getSourceManager());
    return p;
  }
  ADD_FAILURE() << "no function named " << func << " with a parameter";
  return p;
}

} // namespace

// ---------------------------------------------------------------------------
// Viable shapes
// ---------------------------------------------------------------------------

TEST(PlanPointer, CharPointerIsCString) {
  auto p = planFirstParam("void f(char *s) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::CString);
  EXPECT_EQ(p.plan.helper, "__havoc_cstring");
  EXPECT_EQ(p.plan.sizeExpr, "__HAVOC_STR_MAX");
}

TEST(PlanPointer, ConstCharPointerIsStillCString) {
  // Qualifiers on the pointee must not defeat the char check.
  auto p = planFirstParam("void f(const char *s) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::CString);
}

TEST(PlanPointer, ScalarPointerIsBlockSizedByPointee) {
  // No declared bound, so the block is kArrayElems of them - sized by the
  // real pointee type rather than a fixed byte count.
  auto p = planFirstParam("void f(int *a) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Block);
  EXPECT_EQ(p.plan.helper, "__havoc_block");
  EXPECT_EQ(p.plan.sizeExpr, "sizeof(int) * __HAVOC_ARRAY_ELEMS");
}

TEST(PlanPointer, ArrayParamKeepsDeclaredBound) {
  // `int a[3]` decays to `int *` in getType(), but getOriginalType() still
  // carries the ConstantArrayType, so the exact bound is recoverable.
  auto p = planFirstParam("void f(int a[3]) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Array);
  EXPECT_EQ(p.plan.elems, 3u);
  EXPECT_EQ(p.plan.sizeExpr, "sizeof(int) * 3");
}

TEST(PlanPointer, VoidPointerIsOpaqueByteCount) {
  auto p = planFirstParam("void f(void *p) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Opaque);
  EXPECT_EQ(p.plan.sizeExpr, "__HAVOC_OPAQUE_BYTES");
}

TEST(PlanPointer, IncompleteRecordPointerIsOpaque) {
  // Only a forward declaration is visible, so sizeof() is unavailable; the
  // flat byte block is the only safe sizing.
  auto p = planFirstParam("struct Hidden; void f(struct Hidden *p) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Opaque);
  EXPECT_EQ(p.plan.sizeExpr, "__HAVOC_OPAQUE_BYTES");
}

TEST(PlanPointer, PointerFreeRecordIsSizedBySizeof) {
  auto p = planFirstParam("struct Point { int x; int y; };\n"
                          "void f(struct Point *p) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.sizeExpr, "sizeof(struct Point) * __HAVOC_ARRAY_ELEMS");
}

// ---------------------------------------------------------------------------
// Non-viable shapes
//
// Each of these would produce output that either does not compile or violates
// the __VERIFIER_nondet_memory contract, so the classifier must refuse rather
// than guess.
// ---------------------------------------------------------------------------

TEST(PlanPointer, PointerToPointerIsNotViable) {
  // Bulk-havocking a char** fills it with arbitrary nondet pointer values,
  // which the spec forbids dereferencing.
  auto p = planFirstParam("void f(char **argv) {}");
  EXPECT_FALSE(p.plan.viable);
}

TEST(PlanPointer, RecordWithPointerFieldIsNotViable) {
  // Same reason one level down: the `next` field would be a raw nondet
  // pointer. Recursive field initialization is what unlocks this shape.
  auto p = planFirstParam("struct Node { int v; struct Node *next; };\n"
                          "void f(struct Node *n) {}");
  EXPECT_FALSE(p.plan.viable);
}

TEST(PlanPointer, NestedRecordWithPointerFieldIsNotViable) {
  // The pointer-field check is transitive, not just one level deep.
  auto p = planFirstParam("struct Inner { char *name; };\n"
                          "struct Outer { int id; struct Inner inner; };\n"
                          "void f(struct Outer *o) {}");
  EXPECT_FALSE(p.plan.viable);
}

TEST(PlanPointer, FunctionPointerIsNotViable) {
  // There is no nondet value for a function pointer that can be safely called.
  auto p = planFirstParam("void f(int (*cb)(int)) {}");
  EXPECT_FALSE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Function);
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

TEST(RenderPointerExpr, CastsBlockToRequestedType) {
  auto p = planFirstParam("void f(int *a) {}");
  EXPECT_EQ(renderPointerExpr(p.plan, "int *"),
            "(int *)__havoc_block(sizeof(int) * __HAVOC_ARRAY_ELEMS)");
}

TEST(RenderPointerExpr, CStringUsesStringHelper) {
  auto p = planFirstParam("void f(char *s) {}");
  EXPECT_EQ(renderPointerExpr(p.plan, "char *"), "(char *)__havoc_cstring(__HAVOC_STR_MAX)");
}
