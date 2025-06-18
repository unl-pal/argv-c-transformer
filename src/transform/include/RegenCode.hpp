#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>

class RegenCodeVisitor : public clang::RecursiveASTVisitor<RegenCodeVisitor> {
public:
	RegenCodeVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output);

	virtual bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *D);

	virtual bool VisitDecl(clang::Decl *D);

	virtual bool VisitFunctionDecl(clang::FunctionDecl *D);

	virtual bool VisitVarDecl(clang::VarDecl *D);

	virtual bool VisitRecordDecl(clang::RecordDecl *D);

	virtual bool VisitTypedefDecl(clang::TypedefDecl *D);

	virtual bool VisitUnnamedGlobalConstantDecl(clang::UnnamedGlobalConstantDecl *D);

	virtual bool VisitParmVarDecl(clang::ParmVarDecl *D);

	virtual bool VisitFieldDecl(clang::FieldDecl *D);

	bool shouldTraversePostOrder();

private:
	clang::ASTContext *_C;
	clang::SourceManager &_M;
	llvm::raw_ostream &_Output;
	// Comments are NOT implemented at this time but are planned
	llvm::DenseMap<const clang::Decl*, const clang::RawComment> *_Comments;
};
