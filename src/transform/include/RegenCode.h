#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/CodeGenOptions.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

class RegenCodeVisitor : public clang::RecursiveASTVisitor<RegenCodeVisitor> {
public:
	RegenCodeVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output);

	bool VisitStmt(clang::Stmt *S);

	bool VisitDecl(clang::Decl *D);

	bool VisitFunctionDecl(clang::FunctionDecl *D);

	bool VisitVarDecl(clang::VarDecl *D);

	bool VisitRecordDecl(clang::RecordDecl *D);

private:
	clang::ASTContext *_C;
	clang::CodeGenOptions Opts;
	clang::SourceManager &_Mgr;
	llvm::raw_ostream &_Output;

};
