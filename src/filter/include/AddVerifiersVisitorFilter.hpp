#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <set>
#include <string>

/**
 * @brief Injects {@code extern __VERIFIER_nondet_*} declarations into the source.
 *
 * For each type name in {@code neededTypes}, looks up the corresponding C type
 * string in {@code kCTypeNames} and inserts a declaration as text before the first
 * writable node in the file using the {@code Rewriter}.
 *
 * Does not use {@code RecursiveASTVisitor} — it directly iterates the translation
 * unit's top-level declarations to find the insertion point.
 */
class AddVerifiersVisitorFilter {
public:
  /**
   * @brief Constructs the visitor with the shared pipeline state.
   *
   * @param c            AST context, used to construct new AST nodes and look up types.
   * @param neededTypes  Set of verifier name suffixes (e.g. "int", "uint") to declare.
   * @param rewriter     Shared rewriter; declarations are inserted here.
   */
  AddVerifiersVisitorFilter(clang::ASTContext *c, std::set<std::string> *neededTypes,
                            clang::Rewriter &rewriter);

  /**
   * @brief Inserts verifier declarations before the first node in the translation unit.
   *
   * Finds the first writable (non-macro, in-main-file) declaration to use as
   * the insertion point. If that declaration has a doc comment, inserts before
   * the comment. Builds each declaration as a proper AST node, prints it to a
   * string, and inserts it via the Rewriter.
   *
   * @param D  Root of the translation unit.
   * @return   {@code false} (signals no further traversal needed).
   */
  bool HandleTranslationUnit(clang::TranslationUnitDecl *D);

private:
  clang::ASTContext *_C;
  std::set<std::string> *_NeededTypes;
  clang::Rewriter &_Rewriter;
};
