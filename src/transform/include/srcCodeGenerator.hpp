#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/CodeGenOptions.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

class srcCodeGeneratorVisitor : public clang::RecursiveASTVisitor<srcCodeGeneratorVisitor> {
public:
	// srcCodeGeneratorVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output, std::shared_ptr<clang::Preprocessor> PP);
	srcCodeGeneratorVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output, clang::Preprocessor *PP);

	bool VisitTranslationUnitDecl(clang::TranslationUnitDecl *D);

	bool VisitExternCContextDecl(clang::ExternCContextDecl *D);

	bool VisitStmt(clang::Stmt *S);

	bool VisitDecl(clang::Decl *D);

	bool VisitFunctionDecl(clang::FunctionDecl *D);

	bool VisitVarDecl(clang::VarDecl *D);

	bool VisitRecordDecl(clang::RecordDecl *D);

	bool VisitCallExpr(clang::CallExpr *E);

private:
	clang::ASTContext *_C;
	clang::CodeGenOptions Opts;
	clang::SourceManager &_Mgr;
	llvm::raw_ostream &_Output;
	clang::Preprocessor *_PP;
	clang::DiagnosticsEngine &_Diag;

};
