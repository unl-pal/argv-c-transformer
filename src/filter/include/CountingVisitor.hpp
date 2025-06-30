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

// Visitor for counting propeties and functions in a file
class CountNodesVisitor : public clang::RecursiveASTVisitor<CountNodesVisitor> {
public:
  // Attributes tracked by the visitor for the vector of types
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

  // Stmt does not have get parent function so recurse to decl and use built in
  // from there
	std::string getStmtParentFuncName(const clang::Stmt &S);

	// Take Advantage of built in Decl get Parent function
	std::string getDeclParentFuncName(const clang::Decl &D);

  // Base Visit Decl, currently used as catch all for unhandled decl types
	bool VisitDecl(clang::Decl *D);

  // Visits variable declarations and checks if in main file before adding to the
  // count of variables for the function or program as a whole if defined outside
  // of a function
  // can use the defined outside function to check if part of overall
	bool VisitVarDecl(clang::VarDecl *VD);

  // Visit function declarations and add to the map of functions and attributes
	bool VisitFunctionDecl(clang::FunctionDecl *FD);

  // Visit a declaration reference expression checking for type of variable
  // referenced rather than what specfic variable was referenced
  // DeclRefExpr is an Expression not Declaration
	bool VisitDeclRefExpr(clang::DeclRefExpr *D);

  // Base visit statement call, need to separate out the specific calls if possible
	bool VisitStmt(clang::Stmt *S);

  // check if 'if' statement is in main file, is a part of a function and add to
  // current function count
	bool VisitIfStmt(clang::IfStmt *If);

  // Visit for loops and add to the function count of for loops
	bool VisitForStmt(clang::ForStmt *F);

  // Visit while loops and add to the function count of while loops
	bool VisitWhileStmt(clang::WhileStmt *W);

  // check for operations that involve only one variable or literal
  // TODO match the argv on this
	bool VisitUnaryOperator(clang::UnaryOperator *O);

  // Visit binary operations, operations with a left and right side, and add to
  // the count of total binary operations then check if is a comparison binary
  // operation
	bool VisitBinaryOperator(clang::BinaryOperator *O);

  // Visit conditional operator adding to the parent function count of
  // conditional operations
	bool VisitConditionalOperator(clang::ConditionalOperator *O);

  // Visit conditional operators that have a left and right side then add to the count
	bool VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *O);

  // count the parameters in the function signiture and check if is an int
	bool VisitImplicitParamDecl(clang::ImplicitParamDecl *D);

  // Getter for all functions and their attributes
	std::unordered_map<std::string, attributes*> ReportAttributes();

  // Outdated debugging print statement for the report
	void PrintReport(std::string fileName);

private:
	clang::ASTContext *_C;
	clang::SourceManager *_mgr;
	std::map<std::string, int> _values;
	std::unordered_map<std::string, attributes*> *_allFunctions;
	const std::vector<unsigned int> &_T;
	bool _allTypes;
};
