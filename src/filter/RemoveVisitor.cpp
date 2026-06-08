#include "include/RemoveVisitor.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <unordered_map>
#include <vector>

// Canonical mapping from Clang builtin type kind to SV-Comp verifier name suffix.
// Types not in this map are unsupported and will be skipped.
static const std::unordered_map<clang::BuiltinType::Kind, std::string> kVerifierNames = {
    {clang::BuiltinType::Bool, "bool"},     {clang::BuiltinType::Char_S, "char"},
    {clang::BuiltinType::Char_U, "char"},   {clang::BuiltinType::SChar, "char"},
    {clang::BuiltinType::UChar, "uchar"},   {clang::BuiltinType::Short, "short"},
    {clang::BuiltinType::UShort, "ushort"}, {clang::BuiltinType::Int, "int"},
    {clang::BuiltinType::UInt, "uint"},     {clang::BuiltinType::Long, "long"},
    {clang::BuiltinType::ULong, "ulong"},   {clang::BuiltinType::LongLong, "longlong"},
    {clang::BuiltinType::ULongLong, "ulonglong"}, {clang::BuiltinType::Float, "float"},
    {clang::BuiltinType::Double, "double"},
};

RemoveVisitor::RemoveVisitor(clang::ASTContext *C, clang::Rewriter &rewriter,
                             std::vector<std::string> *toRemove,
                             std::set<std::string> *neededTypes)
    : _C(C), _Mgr(rewriter.getSourceMgr()), _Rewriter(rewriter), _ToRemove(toRemove),
      _NeededTypes(neededTypes) {}

bool RemoveVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (!D)
    return false;
  if (_Mgr.isInMainFile(D->getLocation())) {
    // Macro-expanded locations are not writable by the Rewriter
    if (D->getLocation().isMacroID())
      return clang::RecursiveASTVisitor<RemoveVisitor>::VisitFunctionDecl(D);

    for (std::string &name : *_ToRemove) {
      if (name == D->getNameAsString() && !D->isMain()) {
        // Start at column 1 to capture any leading whitespace on that line
        clang::SourceLocation lineStart = _Mgr.translateLineCol(
            _Mgr.getMainFileID(), _Mgr.getSpellingLineNumber(D->getLocation()), 1);
        clang::SourceRange range(lineStart, D->getEndLoc());

        // For forward declarations getEndLoc() is the ')'; extend by 1 to include ';'
        if (!D->hasBody())
          range.setEnd(D->getEndLoc().getLocWithOffset(1));

        if (range.isValid())
          _Rewriter.RemoveText(range);

        // Remove attached doc comment if present
        clang::RawComment *rawComment = _C->getRawCommentForDeclNoCache(D);
        if (rawComment && rawComment->getSourceRange().isValid())
          _Rewriter.RemoveText(rawComment->getSourceRange());
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveVisitor>::VisitFunctionDecl(D);
}

bool RemoveVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (!_Mgr.isInMainFile(E->getExprLoc()))
    return clang::RecursiveASTVisitor<RemoveVisitor>::VisitCallExpr(E);

  if (clang::Decl *calleeDecl = E->getCalleeDecl()) {
    if (clang::FunctionDecl *func = calleeDecl->getAsFunction()) {
      std::string name = func->getNameAsString();
      clang::QualType returnType = E->getCallReturnType(*_C);

      for (std::string &removedName : *_ToRemove) {
        if (name != removedName)
          continue;

        const clang::BuiltinType *BT = returnType->getAs<clang::BuiltinType>();
        if (!BT)
          continue;

        auto it = kVerifierNames.find(BT->getKind());
        if (it == kVerifierNames.end())
          continue;

        const std::string &verifierTypeName = it->second;
        std::string replacement = "__VERIFIER_nondet_" + verifierTypeName + "()";
        if (E->getSourceRange().isValid()) {
          _Rewriter.ReplaceText(E->getSourceRange(), replacement);
          _NeededTypes->insert(verifierTypeName);
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveVisitor>::VisitCallExpr(E);
}

bool RemoveVisitor::shouldTraversePostOrder() { return false; }
