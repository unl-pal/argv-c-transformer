#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/PreprocessingRecord.h>
#include <string>
#include <unordered_map>

class CountNodesVisitor : public clang::RecursiveASTVisitor<CountNodesVisitor> {
public:

	struct attributes {
		int numberTypeArithmeticOperation = 0;
		int numCallFunc = 0;
		int numCompChar = 0;
		int numCompFloat = 0;
		int numCompInt = 0;
		int numFunctions = 0;
		int numIfStmt = 0;
		int numIfStmtInt = 0;
		int numIntParam = 0;
		int numLoopFor = 0;
		int numLoopWhile = 0;
		int numOpBinary = 0;
		int numOpCompare = 0;
		int numOpCondition = 0;
		int numOpUnary = 0;
		int numParam = 0;
		int numPostfix = 0;
		int numPrefix = 0;
		int numVarFloat = 0;
		int numVarInt = 0;
		int numVarPoint = 0;
		int numVarRefArray = 0;
		int numVarRefCompare = 0;
		int numVarRefInt = 0;
		int numVarRefStruct = 0;
		int numVarStruct = 0;
	};

	CountNodesVisitor(clang::ASTContext *C);

	bool partOfBinCompOp(const clang::Stmt &S);

	std::string getStmtParentFuncName(const clang::Stmt &S);

	std::string getDeclParentFuncName(const clang::Decl &D);

	bool VisitDecl(clang::Decl *D);

	bool VisitVarDecl(clang::VarDecl *VD);

	bool VisitFunctionDecl(clang::FunctionDecl *FD);

	bool VisitDeclRefExpr(clang::DeclRefExpr *D);

	bool VisitStmt(clang::Stmt *S);

	bool VisitIntegerLiteral(clang::IntegerLiteral *S);

	bool VisitIfStmt(clang::IfStmt *If);

	bool VisitForStmt(clang::ForStmt *F);

	bool VisitWhileStmt(clang::WhileStmt *W);

	bool VisitUnaryOperator(clang::UnaryOperator *O);

	bool VisitBinaryOperator(clang::BinaryOperator *O);

	bool VisitConditionalOperator(clang::ConditionalOperator *O);

	bool VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O);

	bool VisitType(clang::Type *T);

	bool VisitImplicitParamDecl(clang::ImplicitParamDecl *D);

	std::unordered_map<std::string, attributes*> ReportAttributes();

	void PrintReport(std::string fileName);

private:
	clang::ASTContext *_C;
	clang::SourceManager *_mgr;
	std::map<std::string, int> _values;
	std::unordered_map<std::string, attributes*> _allFunctions;
	int _isInBinCompOp;
};
