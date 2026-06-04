#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <string>
#include <unordered_map>
#include <vector>

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
  /**
   * @brief Per-function AST property counts.
   *
   * Fields prefixed {@code Type*} are gated on the requested builtin types
   * (e.g. only count {@code TypeVariables} if the variable's type matches one
   * of the configured types). Un-prefixed fields (e.g. {@code ForLoops}) count
   * all occurrences regardless of type.
   */
  struct attributes {
    int CallFunc = 0;
    int ForLoops = 0;
    int Functions = 0;
    int IfStmt = 0;
    int Param = 0;
    int TypeArithmeticOperation = 0;
    int TypeCompareOperation = 0;
    int TypeIfStmt = 0;
    int TypeParameters = 0;
    int TypePostfix = 0;
    int TypePrefix = 0;
    int TypeUnaryOperation = 0;
    int TypeVariableReference = 0;
    int TypeVariables = 0;
    int WhileLoops = 0;
  };

  /**
   * @brief Constructs the visitor and seeds the map with the "Program" entry.
   *
   * @param C             AST context, used for parent-map lookups.
   * @param T             Clang BuiltinType values to count; empty means count
   * all.
   * @param allFunctions  Output map shared with downstream consumers.
   */
  CountingVisitor(clang::ASTContext *C, const std::vector<unsigned int> &T,
                  std::unordered_map<std::string, CountingVisitor::attributes *> *allFunctions);

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

  /** @brief Catch-all for declaration nodes not handled by a more specific
   * visitor. */
  bool VisitDecl(clang::Decl *D);

  /** @brief Counts type-matched variable declarations per function. */
  bool VisitVarDecl(clang::VarDecl *VD);

  /** @brief Registers each function in {@code _allFunctions} and increments the
   * file-level function count. */
  bool VisitFunctionDecl(clang::FunctionDecl *FD);

  /**
   * @brief Counts type-matched variable references per function.
   *
   * {@code DeclRefExpr} is the AST node for any use of a named variable —
   * it is an expression, not a declaration, so it appears under {@code Stmt}
   * in the hierarchy.
   */
  bool VisitDeclRefExpr(clang::DeclRefExpr *D);

  /** @brief Catch-all for statement nodes; counts function calls ({@code
   * CallFunc}). */
  bool VisitStmt(clang::Stmt *S);

  /** @brief Counts all if-statements and type-matched if-statement conditions.
   */
  bool VisitIfStmt(clang::IfStmt *If);

  /** @brief Counts for-loop occurrences per function. */
  bool VisitForStmt(clang::ForStmt *F);

  /** @brief Counts while-loop occurrences per function. */
  bool VisitWhileStmt(clang::WhileStmt *W);

  /**
   * @brief Counts type-matched unary operations, distinguishing arithmetic,
   * prefix, and postfix.
   *
   * Note: {@code VisitStmt} does not double-count these — the {@code
   * UnaryOperatorClass} branch was removed from {@code VisitStmt} to avoid
   * that.
   */
  bool VisitUnaryOperator(clang::UnaryOperator *O);

  /** @brief Counts type-matched additive and comparison binary operations per
   * function. */
  bool VisitBinaryOperator(clang::BinaryOperator *O);

  /** @brief Counts type-matched ternary ({@code x ? y : z}) operations per
   * function. */
  bool VisitConditionalOperator(clang::ConditionalOperator *O);

  /**
   * @brief Stub for GNU ({@code x ?: y}) — not currently counted.
   *
   * The standard ternary is handled by {@code VisitConditionalOperator};
   * this form rarely appears in the filtered source files.
   */
  bool VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O);

  /** @brief Counts type-matched implicit parameters (e.g. {@code self} in ObjC
   * methods). */
  bool VisitImplicitParamDecl(clang::ImplicitParamDecl *D);

  /**
   * @brief Returns true if {@code QT} should be counted given the type filter.
   *
   * When {@code _T} is empty (count-all mode), always returns true without
   * iterating. Otherwise checks each requested builtin type.
   */
  bool matchesType(clang::QualType QT) const;

private:
  clang::ASTContext *_C;
  clang::SourceManager *_mgr;
  std::unordered_map<std::string, attributes *> *_allFunctions;
  const std::vector<unsigned int> &_T;
  bool _allTypes; ///< True when _T is empty — count all types
};
