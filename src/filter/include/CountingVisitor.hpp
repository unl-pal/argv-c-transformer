#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Three-state gate for a feature flag: whether the config requires a
 * function to have the feature present, forbids it, or doesn't care.
 */
enum class FeatureGate { Ignore, Require, Forbid };

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
 * pipeline. The special key {@code "Program"} accumulates counts for anything
 * declared at file scope rather than inside a function.
 */
class CountingVisitor : public clang::RecursiveASTVisitor<CountingVisitor> {
public:
  /** @brief Per-function structural complexity counts — the "how much" axis. */
  struct ComplexityCounts {
    int CallFunc = 0;
    int ForLoops = 0;
    int Functions = 0;
    int IfStmt = 0;
    int Param = 0;
    int WhileLoops = 0;
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
   * @brief Constructs the visitor and seeds the map with the "Program" entry.
   *
   * @param C             AST context, used for parent-map lookups.
   * @param allFunctions  Output map shared with downstream consumers.
   */
  CountingVisitor(
      clang::ASTContext *C,
      std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> allFunctions);

  /**
   * @brief Walks up the parent chain of a {@code Stmt} to find its enclosing
   * function name.
   *
   * {@code Stmt} has no direct {@code getParentFunctionOrMethod()} so this
   * recursively climbs via {@code ASTContext::getParents()} until it reaches a
   * {@code FunctionDecl} or falls back to {@code getDeclParentFuncName}.
   *
   * @param S  Statement whose enclosing function to find.
   * @return   Function name, or {@code "Program"} if at file scope.
   */
  std::string getStmtParentFuncName(const clang::Stmt &S);

  /**
   * @brief Returns the name of the function enclosing a {@code Decl}.
   *
   * Uses the built-in {@code getParentFunctionOrMethod()} available on all
   * {@code Decl} nodes.
   *
   * @param D  Declaration whose enclosing function to find.
   * @return   Function name, or {@code "Program"} if at file scope.
   */
  std::string getDeclParentFuncName(const clang::Decl &D);

  /** @brief Checks calls for chacaracteristics (just concurrency for now)
   * by looking at param types and marking parent/enclosing function as using
   * concurrency
   */
  bool VisitCallExpr(clang::CallExpr *CE);

  /** @brief Catch-all for declaration nodes not handled by a more specific
   * visitor. */
  bool VisitDecl(clang::Decl *D);

  /** @brief Counts variable declarations per function; flags floating-point
   * and concurrency types. */
  bool VisitVarDecl(clang::VarDecl *VD);

  /** @brief Registers each function in {@code _allFunctions}, increments the
   * file-level function count, and flags a floating-point return type. */
  bool VisitFunctionDecl(clang::FunctionDecl *FD);

  /** @brief Catch-all for statement nodes; counts function calls ({@code
   * CallFunc}). */
  bool VisitStmt(clang::Stmt *S);

  /** @brief Counts all if-statements per function. */
  bool VisitIfStmt(clang::IfStmt *If);

  /** @brief Counts for-loop occurrences per function. */
  bool VisitForStmt(clang::ForStmt *F);

  /** @brief Counts while-loop occurrences per function. */
  bool VisitWhileStmt(clang::WhileStmt *W);

private:
  /**
   * @brief Safe accessor for a function's attribute bucket.
   *
   * The various Visit* methods resolve an enclosing function name and increment
   * its counters. That name is not guaranteed to be in {@code _allFunctions}:
   * only functions whose declaration location is in the main file are emplaced
   * (see {@code VisitFunctionDecl}), but a node can pass its own
   * {@code isInMainFile} check while its enclosing function came from a macro
   * expansion or included header. A raw {@code at()} would throw
   * {@code std::out_of_range} and terminate the run. This routes any unknown
   * name to the always-present {@code "Program"} (file-scope) bucket instead.
   *
   * @param name  Enclosing-function name from get*ParentFuncName.
   * @return      Reference to the matching (or fallback) attribute bucket.
   */
  attributes &entryFor(const std::string &name);

  clang::ASTContext *_C;
  clang::SourceManager *_mgr;
  std::shared_ptr<std::unordered_map<std::string, attributes>> _allFunctions;
};
