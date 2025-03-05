#pragma once
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <string>
#include <string_view>
#include <vector>

class CountNodesVisitor : public clang::RecursiveASTVisitor<CountNodesVisitor> {
public:
	CountNodesVisitor(clang::ASTContext *C);

	bool incrementCount(std::vector<std::string_view> fields);

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

	/*bool VisitCompoundAssignOperator(clang::CompoundAssignOperator *O);*/

	/*bool VisitIncrementDecrementOperator(clang::Incr *O);*/

	bool VisitType(clang::Type *T);

	boost::json::object Report();

	void PrintReport();
	void PrintReport(const boost::json::value &jv, std::string indent);

private:
	clang::ASTContext *_C;
	clang::SourceManager *_mgr;
	std::map<std::string, int> _values;
	/*boost::json::object *_allFunctions;*/
	boost::json::object _allFunctions;
	/*std::string _currentFunc;*/
};
