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
  clang::QualType declared; ///< The parameter's pre-decay type the plan was built from.
};

// Parses `code`, finds the first parameter of the function named `func`, and
// classifies its declared (pre-decay) type. The ASTUnit is returned alongside
// the plan because the plan's strings outlive nothing but the AST must stay
// alive for the duration of the parse itself.
Parsed planFirstParam(const std::string &code, unsigned depth = 1,
                      const std::string &func = "f") {
  Parsed p;
  p.ast = clang::tooling::buildASTFromCodeWithArgs(code, {"-xc"}, "test.c");
  EXPECT_NE(p.ast, nullptr) << "AST failed to build for:\n" << code;
  if (!p.ast) return p;

  clang::ASTContext &ctx = p.ast->getASTContext();
  for (clang::Decl *decl : ctx.getTranslationUnitDecl()->decls()) {
    auto *fn = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (!fn || fn->getNameAsString() != func || fn->param_empty()) continue;
    p.declared = fn->getParamDecl(0)->getOriginalType();
    p.plan = planPointer(p.declared, ctx.getSourceManager(), depth);
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
}

TEST(PlanPointer, ArrayParamKeepsDeclaredBound) {
  // `int a[3]` decays to `int *` in getType(), but getOriginalType() still
  // carries the ConstantArrayType, so the exact bound is recoverable.
  auto p = planFirstParam("void f(int a[3]) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Array);
  EXPECT_EQ(p.plan.elems, 3u);
}

TEST(PlanPointer, VoidPointerIsOpaqueByteCount) {
  auto p = planFirstParam("void f(void *p) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Opaque);
}

TEST(PlanPointer, IncompleteRecordPointerIsOpaque) {
  // Only a forward declaration is visible, so sizeof() is unavailable; the
  // flat byte block is the only safe sizing.
  auto p = planFirstParam("struct Hidden; void f(struct Hidden *p) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Opaque);
}

TEST(PlanPointer, IncompleteEnumPointerIsOpaqueWithTag) {
  // An enum is a tag too: an incomplete one is opaque, and its cast needs the
  // tag hoisted just like a struct's, or the harness names a fresh enum type.
  auto p = planFirstParam("enum Color; void f(enum Color *c) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Opaque);
  EXPECT_EQ(p.plan.fwdDecl, "enum Color");
}

TEST(PlanPointer, PointerFreeRecordIsSizedByType) {
  auto p = planFirstParam("struct Point { int x; int y; };\n"
                          "void f(struct Point *p) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Record);
}

// ---------------------------------------------------------------------------
// Recursive initialization: pointers inside the storage become slots, given
// their own storage while the depth lasts and set to 0 past it.
// ---------------------------------------------------------------------------

TEST(PlanPointer, PointerToPointerNullsElementsAtDepthOne) {
  auto p = planFirstParam("void f(char **argv) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Block);
  ASSERT_EQ(p.plan.slots.size(), 1u);
  EXPECT_TRUE(p.plan.slots[0].path.empty());
  EXPECT_EQ(p.plan.slots[0].child, nullptr);
}

TEST(PlanPointer, PointerToPointerGetsStringsAtDepthTwo) {
  auto p = planFirstParam("void f(char **argv) {}", 2);
  EXPECT_TRUE(p.plan.viable);
  ASSERT_EQ(p.plan.slots.size(), 1u);
  ASSERT_NE(p.plan.slots[0].child, nullptr);
  EXPECT_EQ(p.plan.slots[0].child->shape, PointerShape::CString);
}

TEST(PlanPointer, RecordWithPointerFieldNullsItAtDepthOne) {
  auto p = planFirstParam("struct Node { int v; struct Node *next; };\n"
                          "void f(struct Node *n) {}");
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Record);
  ASSERT_EQ(p.plan.slots.size(), 1u);
  EXPECT_EQ(p.plan.slots[0].path, std::vector<std::string>{".next"});
  EXPECT_EQ(p.plan.slots[0].child, nullptr);
}

TEST(PlanPointer, SelfReferenceIsBoundedByDepth) {
  // A linked list comes out exactly `depth` nodes deep, then 0.
  auto p = planFirstParam("struct Node { int v; struct Node *next; };\n"
                          "void f(struct Node *n) {}",
                          3);
  const PointerPlan *level = &p.plan;
  for (int i = 0; i < 2; ++i) {
    ASSERT_EQ(level->slots.size(), 1u);
    ASSERT_NE(level->slots[0].child, nullptr) << "level " << i;
    level = level->slots[0].child.get();
  }
  ASSERT_EQ(level->slots.size(), 1u);
  EXPECT_EQ(level->slots[0].child, nullptr);
}

TEST(PlanPointer, NestedRecordFieldPathsThroughTheMember) {
  auto p = planFirstParam("struct Inner { char *name; };\n"
                          "struct Outer { int id; struct Inner inner; };\n"
                          "void f(struct Outer *o) {}",
                          2);
  EXPECT_TRUE(p.plan.viable);
  ASSERT_EQ(p.plan.slots.size(), 1u);
  EXPECT_EQ(p.plan.slots[0].path, (std::vector<std::string>{".inner", ".name"}));
  ASSERT_NE(p.plan.slots[0].child, nullptr);
  EXPECT_EQ(p.plan.slots[0].child->shape, PointerShape::CString);
}

TEST(PlanPointer, PointerArrayFieldCarriesItsBound) {
  auto p = planFirstParam("struct Names { char *names[4]; };\n"
                          "void f(struct Names *n) {}");
  ASSERT_EQ(p.plan.slots.size(), 1u);
  EXPECT_EQ(p.plan.slots[0].path, (std::vector<std::string>{".names", "[]"}));
  EXPECT_EQ(p.plan.slots[0].dims, std::vector<uint64_t>{4});
}

TEST(PlanPointer, FunctionPointerFieldIsNotViable) {
  // Nothing callable can be synthesized for it (see docs/FunctionPointerHavocking.md).
  auto p = planFirstParam("struct Ops { int (*cb)(int); };\n"
                          "void f(struct Ops *o) {}",
                          3);
  EXPECT_FALSE(p.plan.viable);
}

TEST(PlanPointer, ArrayOfFunctionPointersIsNotViable) {
  auto p = planFirstParam("void f(int (*fs[2])(int)) {}", 2);
  EXPECT_FALSE(p.plan.viable);
}

TEST(PlanPointer, UnionPointerMembersAreLeftAlone) {
  // Assigning one member would clobber the bytes of the others.
  auto p = planFirstParam("union U { int i; char *s; };\n"
                          "void f(union U *u) {}",
                          2);
  EXPECT_TRUE(p.plan.viable);
  EXPECT_TRUE(p.plan.slots.empty());
}

TEST(PlanPointer, SelfContainingRecordFromErrorRecoveryTerminates) {
  // Typo correction resolves the undeclared GtkWindow to YuiWindow, so the record contains
  // itself by value.
  auto ast = clang::tooling::buildASTFromCodeWithArgs(
      "typedef struct _YuiWindow YuiWindow;\n"
      "struct _YuiWindow { GtkWindow hbox; char *name; };\n"
      "void f(YuiWindow *w) {}",
      {"-xc", "-Wno-everything"}, "test.c");
  ASSERT_NE(ast, nullptr);
  for (clang::Decl *decl : ast->getASTContext().getTranslationUnitDecl()->decls()) {
    auto *fn = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (!fn || fn->getNameAsString() != "f") continue;
    planPointer(fn->getParamDecl(0)->getOriginalType(), ast->getASTContext().getSourceManager(), 2);
    return; // reaching here at all is the assertion
  }
  ADD_FAILURE() << "no f";
}

TEST(PlanPointer, ConstPointerFieldIsNotViable) {
  // Can't be assigned after the fill, so it would stay a raw nondet pointer.
  auto p = planFirstParam("struct R { char *const name; };\n"
                          "void f(struct R *r) {}");
  EXPECT_FALSE(p.plan.viable);
}

// ---------------------------------------------------------------------------
// Depth 0: no structure modelled, everything but strings is opaque bytes.
// ---------------------------------------------------------------------------

TEST(PlanPointerDepthZero, CharPointerStaysCString) {
  auto p = planFirstParam("void f(char *s) {}", 0);
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::CString);
}

TEST(PlanPointerDepthZero, RecordWithPointersIsOpaqueFlooredBySizeof) {
  auto p = planFirstParam("struct Node { int v; struct Node *next; };\n"
                          "void f(struct Node *n) {}",
                          0);
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Opaque);
  EXPECT_EQ(p.plan.opaqueSizeof, "struct Node");
  EXPECT_TRUE(p.plan.slots.empty());
}

TEST(PlanPointerDepthZero, PointerToPointerIsOpaque) {
  auto p = planFirstParam("void f(char **argv) {}", 0);
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Opaque);
}

TEST(PlanPointerDepthZero, ArrayParamFlooredByWholeArray) {
  auto p = planFirstParam("void f(int a[3]) {}", 0);
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Opaque);
  EXPECT_EQ(p.plan.opaqueSizeof, "int[3]");
}

TEST(PlanPointerDepthZero, ArrayOfFunctionPointersSpellsItsSizeof) {
  // The bound nests inside the declarator; appending it would name a function returning an array.
  auto p = planFirstParam("void f(int (*fs[2])(int)) {}", 0);
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.opaqueSizeof, "int (*[2])(int)");
}

TEST(PlanPointerDepthZero, MultiDimArrayKeepsBoundOrder) {
  auto p = planFirstParam("void f(int g[3][4]) {}", 0);
  EXPECT_EQ(p.plan.opaqueSizeof, "int[3][4]");
}

TEST(PlanPointerDepthZero, CStringHasNoSizeofFloor) {
  auto p = planFirstParam("void f(char *s) {}", 0);
  EXPECT_EQ(p.plan.opaqueSizeof, "");
}

TEST(PlanPointerDepthZero, IncompleteRecordHasNoSizeofFloor) {
  auto p = planFirstParam("struct Hidden; void f(struct Hidden *p) {}", 0);
  EXPECT_TRUE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Opaque);
  EXPECT_EQ(p.plan.opaqueSizeof, "");
  EXPECT_EQ(p.plan.fwdDecl, "struct Hidden");
}

TEST(PlanPointerDepthZero, FunctionPointerIsStillNotViable) {
  auto p = planFirstParam("void f(int (*cb)(int)) {}", 0);
  EXPECT_FALSE(p.plan.viable);
}

TEST(PlanPointer, AnonymousRecordPointerIsNotViable) {
  // No spelling for the storage declaration or the harness cast, at any depth.
  for (unsigned depth : {0u, 1u, 2u}) {
    auto p = planFirstParam("void f(struct { int x; } *p) {}", depth);
    EXPECT_FALSE(p.plan.viable) << "depth " << depth;
  }
}

TEST(PlanPointer, TypedefedAnonymousRecordIsViable) {
  auto p = planFirstParam("typedef struct { int x; } S;\nvoid f(S *p) {}");
  EXPECT_TRUE(p.plan.viable);
}

TEST(PlanPointer, FunctionPointerIsNotViable) {
  // There is no nondet value for a function pointer that can be safely called.
  auto p = planFirstParam("void f(int (*cb)(int)) {}");
  EXPECT_FALSE(p.plan.viable);
  EXPECT_EQ(p.plan.shape, PointerShape::Function);
}

// ---------------------------------------------------------------------------
// Rendering: stack storage filled with __VERIFIER_nondet_memory, no heap.
// ---------------------------------------------------------------------------

TEST(RenderPointerStorage, BlockDeclaresTypedArrayAndPassesIt) {
  auto p = planFirstParam("void f(int *a) {}");
  PointerStorage s = renderPointerStorage(p.plan, p.declared, "__h0", "int *");
  EXPECT_EQ(s.arg, "__h0");
  EXPECT_FALSE(s.cstring);
  EXPECT_EQ(s.decls, "  int __h0[__HAVOC_ARRAY_ELEMS];\n"
                     "  __VERIFIER_nondet_memory(__h0, sizeof(__h0));\n");
}

TEST(RenderPointerStorage, ArrayUsesDeclaredBound) {
  auto p = planFirstParam("void f(int a[3]) {}");
  PointerStorage s = renderPointerStorage(p.plan, p.declared, "__h0", "int *");
  EXPECT_EQ(s.decls, "  int __h0[3];\n"
                     "  __VERIFIER_nondet_memory(__h0, sizeof(__h0));\n");
}

TEST(RenderPointerStorage, CStringPlantsInBoundsTerminator) {
  // The fill and in-bounds terminator both come from argv_c_harness.h's
  // __havoc_cstring_fill helper, which hands back the same buffer, so it
  // stands in for the call directly - only the declaration is hoisted.
  auto p = planFirstParam("void f(char *s) {}");
  PointerStorage s = renderPointerStorage(p.plan, p.declared, "__h0", "char *");
  EXPECT_TRUE(s.cstring);
  EXPECT_EQ(s.arg, "__havoc_cstring_fill(__h0, __HAVOC_STR_MAX)");
  EXPECT_EQ(s.decls, "  char __h0[__HAVOC_STR_MAX];\n");
}

TEST(RenderPointerStorage, OpaqueUsesAlignedByteBufferAndCasts) {
  auto p = planFirstParam("void f(void *p) {}");
  PointerStorage s = renderPointerStorage(p.plan, p.declared, "__h0", "void *");
  EXPECT_EQ(s.arg, "(void *)__h0");
  EXPECT_EQ(s.decls, "  unsigned char __h0[__HAVOC_BLOCK_MAX];\n"
                     "  __VERIFIER_nondet_memory(__h0, sizeof(__h0));\n");
}

TEST(RenderPointerStorage, IndentParameterPrefixesEveryLine) {
  auto p = planFirstParam("void f(int *a) {}");
  PointerStorage s = renderPointerStorage(p.plan, p.declared, "__h0", "int *", "");
  EXPECT_EQ(s.decls, "int __h0[__HAVOC_ARRAY_ELEMS];\n"
                     "__VERIFIER_nondet_memory(__h0, sizeof(__h0));\n");
}

TEST(RenderPointerStorage, PointerToPointerNullsEachElementAtDepthOne) {
  auto p = planFirstParam("void f(char **argv) {}");
  PointerStorage s = renderPointerStorage(p.plan, p.declared, "__h0", "char **");
  EXPECT_EQ(s.arg, "__h0");
  EXPECT_EQ(s.decls, "  char *__h0[__HAVOC_ARRAY_ELEMS];\n"
                     "  __VERIFIER_nondet_memory(__h0, sizeof(__h0));\n"
                     "  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)\n"
                     "    __h0[__i0] = 0;\n");
}

TEST(RenderPointerStorage, LinkedListGetsARowPerElement) {
  auto p = planFirstParam("struct Node { int v; struct Node *next; };\n"
                          "void f(struct Node *n) {}",
                          2);
  PointerStorage s = renderPointerStorage(p.plan, p.declared, "__h0", "struct Node *");
  EXPECT_EQ(s.decls, "  struct Node __h0[__HAVOC_ARRAY_ELEMS];\n"
                     "  __VERIFIER_nondet_memory(__h0, sizeof(__h0));\n"
                     "  struct Node __h0_0[__HAVOC_ARRAY_ELEMS][__HAVOC_ARRAY_ELEMS];\n"
                     "  __VERIFIER_nondet_memory(__h0_0, sizeof(__h0_0));\n"
                     "  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)\n"
                     "    for (int __i1 = 0; __i1 < __HAVOC_ARRAY_ELEMS; ++__i1)\n"
                     "      __h0_0[__i0][__i1].next = 0;\n"
                     "  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)\n"
                     "    __h0[__i0].next = __h0_0[__i0];\n");
}

TEST(RenderPointerStorage, NestedStringFieldIsFilledPerRow) {
  auto p = planFirstParam("struct Names { char *names[4]; };\n"
                          "void f(struct Names *n) {}",
                          2);
  PointerStorage s = renderPointerStorage(p.plan, p.declared, "__h0", "struct Names *");
  EXPECT_EQ(s.decls,
            "  struct Names __h0[__HAVOC_ARRAY_ELEMS];\n"
            "  __VERIFIER_nondet_memory(__h0, sizeof(__h0));\n"
            "  char __h0_0[__HAVOC_ARRAY_ELEMS][4][__HAVOC_STR_MAX];\n"
            "  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)\n"
            "    for (int __i1 = 0; __i1 < 4; ++__i1)\n"
            "      __h0[__i0].names[__i1] = __havoc_cstring_fill(__h0_0[__i0][__i1], __HAVOC_STR_MAX);\n");
}

TEST(RenderPointerStorage, OpaqueSizeofFloorsTheBlock) {
  PointerPlan plan;
  plan.shape = PointerShape::Opaque;
  plan.viable = true;
  plan.opaqueSizeof = "struct Big";
  PointerStorage s = renderPointerStorage(plan, {}, "__h0", "struct Big *");
  EXPECT_EQ(s.arg, "(struct Big *)__h0");
  EXPECT_EQ(s.decls, "  unsigned char __h0[sizeof(struct Big) > __HAVOC_BLOCK_MAX ? sizeof(struct Big) "
                     ": __HAVOC_BLOCK_MAX];\n"
                     "  __VERIFIER_nondet_memory(__h0, sizeof(__h0));\n");
}
