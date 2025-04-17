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
		int TypeArithmeticOperation = 0;
		int CallFunc = 0;
		int CompChar = 0;
		int CompFloat = 0;
		int TypeComparisons = 0;
		int Functions = 0;
		int IfStmt = 0;
		int TypeIfStmt = 0;
		int TypeParameters = 0;
		int ForLoops = 0;
		int WhileLoops = 0;
		int OpBinary = 0;
		int OpCompare = 0;
		int OpCondition = 0;
		int OpUnary = 0;
		int Param = 0;
		int Postfix = 0;
		int Prefix = 0;
		int VarFloat = 0;
		int TypeVariables = 0;
		int VarPoint = 0;
		int VarRefArray = 0;
		int VarRefCompare = 0;
		int TypeVariableReference = 0;
		int VarRefStruct = 0;
		int StructVariable = 0;
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
