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
#include <vector>

class CountNodesVisitor : public clang::RecursiveASTVisitor<CountNodesVisitor> {
public:
	
	struct attributes {
		int CallFunc = 0;
		int ForLoops = 0;
		int Functions = 0;
		int IfStmt = 0;
		int Param = 0;
		int TypeArithmeticOperation = 0;
		int TypeCompareOperation = 0;
		int TypeComparisons = 0;
		int TypeIfStmt = 0;
		int TypeParameters = 0;
		int TypePostfix = 0;
		int TypePrefix = 0;
		int TypeUnaryOperation = 0;
		int TypeVariableReference = 0;
		int TypeVariables = 0;
		int WhileLoops = 0;
	};

  CountNodesVisitor(
    clang::ASTContext *C, const std::vector<unsigned int> &T,
    std::unordered_map<std::string, CountNodesVisitor::attributes *> *allFunctions);

  std::string getStmtParentFuncName(const clang::Stmt &S);

	std::string getDeclParentFuncName(const clang::Decl &D);

	bool VisitDecl(clang::Decl *D);

	bool VisitVarDecl(clang::VarDecl *VD);

	bool VisitFunctionDecl(clang::FunctionDecl *FD);

	bool VisitDeclRefExpr(clang::DeclRefExpr *D);

	bool VisitStmt(clang::Stmt *S);

	bool VisitIfStmt(clang::IfStmt *If);

	bool VisitForStmt(clang::ForStmt *F);

	bool VisitWhileStmt(clang::WhileStmt *W);

	bool VisitUnaryOperator(clang::UnaryOperator *O);

	bool VisitBinaryOperator(clang::BinaryOperator *O);

	bool VisitConditionalOperator(clang::ConditionalOperator *O);

	bool VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O);

	bool VisitImplicitParamDecl(clang::ImplicitParamDecl *D);

	std::unordered_map<std::string, attributes*> ReportAttributes();

	void PrintReport(std::string fileName);

private:
	clang::ASTContext *_C;
	clang::SourceManager *_mgr;
	std::map<std::string, int> _values;
	std::unordered_map<std::string, attributes*> *_allFunctions;
	const std::vector<unsigned int> &_T;
	bool _allTypes;
};
