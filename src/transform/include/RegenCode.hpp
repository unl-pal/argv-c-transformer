#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>

class RegenCodeVisitor : public clang::RecursiveASTVisitor<RegenCodeVisitor> {
public:
	RegenCodeVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output);

	bool VisitDecl(clang::Decl *D);

	bool VisitFunctionDecl(clang::FunctionDecl *D);

	bool VisitVarDecl(clang::VarDecl *D);

	bool VisitRecordDecl(clang::RecordDecl *D);

	bool VisitTypedefDecl(clang::TypedefDecl *D);

	bool VisitUnnamedGlobalConstantDecl(clang::UnnamedGlobalConstantDecl *D);

	bool VisitParmVarDecl(clang::ParmVarDecl *D);

	bool VisitFieldDecl(clang::FieldDecl *D);

private:
	clang::ASTContext *_C;
	clang::SourceManager &_M;
	llvm::raw_ostream &_Output;
	// Comments are NOT implemented at this time but are planned
	llvm::DenseMap<const clang::Decl*, const clang::RawComment> *_Comments;
};
