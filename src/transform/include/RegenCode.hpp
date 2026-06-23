#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>

/**
 * @brief RecursiveASTVisitor that pretty-prints the AST back to C source.
 *
 * Iterates through all top-level nodes and prints them to the output stream.
 * Currently handles functions, variables, records (structs/unions), and typedefs.
 * Comment preservation within function bodies and structs, as well as trailing
 * comments, is not yet implemented.
 */
class RegenCodeVisitor : public clang::RecursiveASTVisitor<RegenCodeVisitor> {
public:
  /**
   * @brief Constructs the visitor with the AST context and output stream.
   *
   * @param C      AST context, used for source manager and comment lookups.
   * @param output File stream to write the regenerated source code to.
   */
  RegenCodeVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output);

  // bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *D);

  /**
   * @brief Base visit called at the end of all declarations after specific visits.
   *
   * @param D The declaration being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  bool VisitDecl(clang::Decl *D);

  /**
   * @brief Visits and prints function declarations to the output.
   *
   * @param D The function declaration being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  bool VisitFunctionDecl(clang::FunctionDecl *D);

  /**
   * @brief Visits and prints variable declarations to the output.
   *
   * @param D The variable declaration being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  bool VisitVarDecl(clang::VarDecl *D);

  /**
   * @brief Visits and prints record types (structs, unions) to the output.
   *
   * @param D The record declaration being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  bool VisitRecordDecl(clang::RecordDecl *D);

  /**
   * @brief Visits and prints typedef declarations to the output.
   *
   * @param D The typedef declaration being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  bool VisitTypedefDecl(clang::TypedefDecl *D);

  /**
   * @brief Visits and prints unnamed global constant declarations to the output.
   *
   * @param D The unnamed global constant declaration being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  bool VisitUnnamedGlobalConstantDecl(clang::UnnamedGlobalConstantDecl *D);

  /**
   * @brief Skips parameter variable printing, deferring to the function visit.
   *
   * @param D The parameter declaration being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  bool VisitParmVarDecl(clang::ParmVarDecl *D);

  /**
   * @brief Skips field declaration printing, deferring to the record visit.
   *
   * @param D The field declaration being visited.
   * @return {@code false} to stop traversal, {@code true} to continue.
   */
  bool VisitFieldDecl(clang::FieldDecl *D);

  /**
   * @brief Instructs the visitor to use post-order (depth-first) traversal.
   *
   * @return {@code true} for post-order traversal.
   */
  bool shouldTraversePostOrder();

private:
  clang::ASTContext *_C;
  clang::SourceManager &_M;
  llvm::raw_ostream &_Output;
  // Comments are NOT implemented at this time but are planned
  llvm::DenseMap<const clang::Decl *, const clang::RawComment> *_Comments;
};
