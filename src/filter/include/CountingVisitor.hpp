// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

/**
 * @brief Recursively walks the AST and counts per-function properties.
 *
 * Uses CRTP ({@code RecursiveASTVisitor<CountingVisitor>}) so the base class
 * can dispatch to our {@code Visit*} overrides at compile time without virtual
 * calls. Each {@code Visit*} method increments counts in {@code _allFunctions},
 * then calls the parent implementation to continue the traversal. Returning
 * {@code false} from any {@code Visit*} stops the entire walk.
 *
 * Results are written into the {@code _allFunctions} map passed at construction
 * — the same map that {@code FilterFunctionsConsumer} reads next in the
 * pipeline. The special key {@code "FileScope"} accumulates counts for anything
 * declared at file scope rather than inside a function.
 */
class CountingVisitor : public clang::RecursiveASTVisitor<CountingVisitor> {
public:
  /** @brief Per-function structural complexity counts — the "how much" axis. */
  struct ComplexityCounts {
    int CallFunc = 0;
    int ForLoops = 0;
    int IfStmt = 0;
    int Param = 0;
    int WhileLoops = 0;
    int Operations = 0; // ops w/side-effects on signed types (for UB/no-overflow property detection)
  };

  /** @brief Per-function feature presence flags — the "what kind" axis. */
  struct FeatureFlags {
    bool Concurrency = false;
    bool FloatingPoint = false;
  };

  /** @brief Per-function AST property counts, split across the two axes. */
  struct attributes {
    ComplexityCounts Complexity;
    FeatureFlags Features;
  };

  /**
   * @brief Constructs the visitor and seeds the map with the "FileScope" entry.
   *
   * @param C             AST context, used for parent-map lookups.
   * @param allFunctions  Output map shared with downstream consumers.
   */
  CountingVisitor(
      clang::ASTContext *C,
      std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> allFunctions);

  /**
   * @brief Walks up the parent chain of a {@code Stmt} to find its enclosing
   * function name, via {@code ASTContext::getParents()} ({@code Stmt} has no
   * direct {@code getParentFunctionOrMethod()}).
   *
   * @param S  Statement whose enclosing function to find.
   * @return   Function name, or {@code "FileScope"} if at file scope.
   */
  std::string getStmtParentFuncName(const clang::Stmt &S);

  /**
   * @brief Returns the name of the function enclosing a {@code Decl}.
   *
   * Uses the built-in {@code getParentFunctionOrMethod()} available on all
   * {@code Decl} nodes.
   *
   * @param D  Declaration whose enclosing function to find.
   * @return   Function name, or {@code "FileScope"} if at file scope.
   */
  std::string getDeclParentFuncName(const clang::Decl &D);

  /** @brief Counts function calls ({@code CallFunc}) and checks call
   * arguments for characteristics (just concurrency for now), marking the
   * enclosing function accordingly. */
  bool VisitCallExpr(clang::CallExpr *CE);

  /** @brief Counts variable declarations per function; flags floating-point
   * and concurrency types. */
  bool VisitVarDecl(clang::VarDecl *VD);

  /** @brief Registers each function in {@code _allFunctions}, increments the
   * file-level function count, and flags a floating-point return type. */
  bool VisitFunctionDecl(clang::FunctionDecl *FD);

  /** @brief Counts all if-statements per function. */
  bool VisitIfStmt(clang::IfStmt *If);

  /** @brief Counts for-loop occurrences per function. */
  bool VisitForStmt(clang::ForStmt *F);

  /** @brief Counts while-loop occurrences per function. */
  bool VisitWhileStmt(clang::WhileStmt *W);

  /** @brief Counts signed binary operations per function. */
  bool VisitBinaryOperator(clang::BinaryOperator *BO);

  /** @brief Counts signed unary operations per function. */
  bool VisitUnaryOperator(clang::UnaryOperator *UO);

private:
  clang::ASTContext *_C;
  clang::SourceManager *_mgr;
  std::shared_ptr<std::unordered_map<std::string, attributes>> _allFunctions;
};

/**
 * @brief Looks up a named field on the complexity axis, shared by the
 * filter's threshold check and the verify stage's post-transform re-check.
 * Throws if `name` isn't one of the known metrics — a config key typo should
 * surface loudly, not silently no-op.
 */
inline int complexityField(const CountingVisitor::ComplexityCounts &c, const std::string &name) {
  if (name == "CallFunc")
    return c.CallFunc;
  if (name == "ForLoops")
    return c.ForLoops;
  if (name == "IfStmt")
    return c.IfStmt;
  if (name == "Param")
    return c.Param;
  if (name == "WhileLoops")
    return c.WhileLoops;
  if (name == "Operations")
    return c.Operations;
  throw std::invalid_argument("unknown complexity metric: " + name);
}

/**
 * @brief Looks up a named flag on the feature axis. Throws if `name` isn't
 * one of the known features, for the same reason as complexityField above.
 */
inline bool featureField(const CountingVisitor::FeatureFlags &f, const std::string &name) {
  if (name == "Concurrency")
    return f.Concurrency;
  if (name == "FloatingPoint")
    return f.FloatingPoint;
  throw std::invalid_argument("unknown feature: " + name);
}
