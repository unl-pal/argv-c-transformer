#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>

class RegenCodeVisitor : public clang::RecursiveASTVisitor<RegenCodeVisitor> {
public:
	/// Visitor currently iterates through all top level nodes and prints the code
	/// This will need to be updated to a more fine tuned version to handle more
	/// complex code and retain comments within function and structs as well as
	/// trailing comments
	RegenCodeVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output);

	// bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *D);

	/// Base visit called at the end of all declarations after specific visits have run
	bool VisitDecl(clang::Decl *D);

	/// Visits and prints Functions to the output
	bool VisitFunctionDecl(clang::FunctionDecl *D);

	/// Visits and prints Variables to the output
	bool VisitVarDecl(clang::VarDecl *D);

	/// Visits and prints Records such as Unions and Structs to the output
	bool VisitRecordDecl(clang::RecordDecl *D);

	/// Visits and prints TypeDefs to the output
	bool VisitTypedefDecl(clang::TypedefDecl *D);

	/// Visits and prints Globals to the output
	bool VisitUnnamedGlobalConstantDecl(clang::UnnamedGlobalConstantDecl *D);

  /// Visits prevents the printing of Parameter Variables to the output letting
  /// the Function visit handle instead
  bool VisitParmVarDecl(clang::ParmVarDecl *D);

  /// Visits prevents the printing of Field Variables to the output letting
  /// the Record visit handle instead
	bool VisitFieldDecl(clang::FieldDecl *D);

	/// Tells the visitor wether to run depth or breadth first on traversal
	bool shouldTraversePostOrder();

private:
	clang::ASTContext *_C;
	clang::SourceManager &_M;
	llvm::raw_ostream &_Output;
	// Comments are NOT implemented at this time but are planned
	llvm::DenseMap<const clang::Decl*, const clang::RawComment> *_Comments;
};
