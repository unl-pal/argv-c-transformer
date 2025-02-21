#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <string>
#include <unordered_map>

class CountNodesVisitor : public clang::RecursiveASTVisitor<CountNodesVisitor> {
public:
	CountNodesVisitor(clang::ASTContext *C);

	bool VisitDecl(clang::Decl *D);

	bool VisitVarDecl(clang::VarDecl *VD);

	bool VisitStmt(clang::Stmt *S);

	bool VisitIfStmt(clang::IfStmt *If);

	bool VisitForStmt(clang::ForStmt *F);

	bool VisitWhileStmt(clang::WhileStmt *W);

	bool VisitType(clang::Type *T);

	std::unordered_map<std::string, int> report();

	/*void PrintReport(std::unordered_map<std::string, int> report);*/
	void PrintReport();

private:
	clang::ASTContext *_C;
	clang::SourceManager *_mgr;
	int _numIfStmt;
	int _numInts;
	int _numLoops;
	int _numOperations;
	int _numPointers;
};
