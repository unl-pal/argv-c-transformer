#include "AddMainVisitor.hpp"
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <vector>

AddMainVisitor::AddMainVisitor(clang::ASTContext       *c,
                               std::vector<std::string> functionsToCall,
                               clang::Rewriter         &r)
    : _Context(c), _FunctionsToCall(functionsToCall), _Rewriter(r) {}

bool AddMainVisitor::VisitTranslationUnitDecl(clang::TranslationUnitDecl *D) {
  if (!D) return false;

  // std::string returnTypeName = returnType.getAsString();
  // llvm::outs() << returnTypeName << "\n";
  // std::replace(returnTypeName.begin(), returnTypeName.end(), ' ', '_');
  // for (clang::Decl *decl : D->)
  clang::SourceLocation loc = D->decls_end()->getPreviousDecl()->getLocation();
  clang::IdentifierInfo *funcName = &_Context->Idents.get("main");
  clang::DeclarationName declName(funcName);
  clang::FunctionProtoType::ExtProtoInfo epi;
  clang::QualType funcQualType = _Context->IntTy;

  clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
    *_Context,
    D,
    loc,
    loc,
    declName,
    funcQualType,
    nullptr,
    clang::SC_Extern
  );
  newFunction->setReferenced();
  newFunction->setIsUsed();
  newFunction->setWillHaveBody(true);
  D->addDecl(newFunction);
  _Rewriter.InsertTextAfter(loc, newFunction->getReturnType().getAsString() + " " + newFunction->getNameAsString() + "() {\n");
  clang::SourceRange range;
  loc = newFunction->getLocation();
  range.setBegin(loc);

  clang::Stmt *body = newFunction->getBody();
  for (std::string name : _FunctionsToCall) {
    clang::IdentifierInfo *funcCallName = &_Context->Idents.get(name);
    clang::DeclarationName funcCallNameString(funcCallName);
    clang::FunctionDecl *func = D->lookup(funcCallNameString).find_first<clang::FunctionDecl>();
    _Rewriter.InsertTextAfter(loc, func->getNameAsString() + "(");
    int i = 0;
    std::string verifierName = "__Verifier_non_det_";
    newFunction->getBody();
    for (clang::ParmVarDecl *parm : func->parameters()) {
      clang::ValueDecl *tempDecl = func;
      clang::DeclRefExpr *call = clang::DeclRefExpr::Create(*_Context, func->getQualifierLoc(), loc, tempDecl, false, loc, parm->getType(), clang::ExprValueKind::VK_LValue);
      body->addStmtClass(call->getStmtClass());
      std::string comma;
      i++? comma = ", " : comma = "";
      std::string tempName = comma + verifierName + std::string(parm->getType()->getTypeClassName());
      _Rewriter.InsertTextAfter(loc, tempName);
      loc = loc.getLocWithOffset(tempName.size());
    }
    _Rewriter.InsertText(loc, ");\n");
    // loc.isPairOfFileLocations(SourceLocation Start, SourceLocation End)
  }
  _Rewriter.InsertTextAfter(loc, "}");
  // loc = loc.getLocWithOffset(tempName.size());

  // return clang::RecursiveASTVisitor<AddMainVisitor>::VisitTranslationUnitDecl(D);
  return true;
}
