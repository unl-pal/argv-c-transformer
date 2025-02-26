#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <string>

class CountNodesVisitor : public clang::RecursiveASTVisitor<CountNodesVisitor> {
public:
	CountNodesVisitor(clang::ASTContext *C);

	bool VisitDecl(clang::Decl *D);

	bool VisitVarDecl(clang::VarDecl *VD);

	bool VisitFunctionDecl(clang::FunctionDecl *FD);

	bool VisitStmt(clang::Stmt *S);

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

	std::map<std::string, int> report();

	/*void PrintReport(std::unordered_map<std::string, int> report);*/
	void PrintReport();

private:
	clang::ASTContext *_C;
	clang::SourceManager *_mgr;
	std::map<std::string, int> _values;
	std::map<std::string, std::map<std::string, int>> _allFunctions;
	std::string _currentFunc;
};
