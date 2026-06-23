#include "include/HavocCallsVisitor.hpp"

#include "VerifierNames.hpp"

#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <optional>

HavocCallsVisitor::HavocCallsVisitor(clang::ASTContext *C,
                                     std::shared_ptr<std::set<std::string>> neededSuffixes,
                                     clang::Rewriter &rewriter)
    : _C(C), _NeededSuffixes(neededSuffixes), _Rewriter(rewriter) {};

bool HavocCallsVisitor::VisitTranslationUnit(clang::TranslationUnitDecl *D) {
  return clang::RecursiveASTVisitor<HavocCallsVisitor>::TraverseDecl(D);
}

bool HavocCallsVisitor::VisitDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitDecl(D);
}

// Havoc every call to a function from this file so each function body is
// self-contained (intraprocedural): the call's value is replaced by a fresh
// nondet of its return type. Library calls (callee declared in a header,
// e.g. the C standard library) are kept as-is.
bool HavocCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  clang::SourceManager &mgr = _C->getSourceManager();
  clang::SourceLocation loc = E->getExprLoc();
  // Only rewrite calls spelled out in the file being transformed; a macro
  // expansion has no rewritable source range of its own
  if (!mgr.isInMainFile(loc) || loc.isMacroID())
    return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);

  if (const clang::FunctionDecl *callee = E->getDirectCallee()) {
    // Keep nondet calls already injected by the filter step
    if (callee->getIdentifier() && callee->getName().starts_with("__VERIFIER_"))
      return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);
    // Keep calls into system headers (the C standard library and other
    // platform headers). Everything else is project code and gets havocked:
    // functions from this file, extern declarations written here, functions
    // declared in repo-local headers, and implicit declarations (calls with
    // no prototype in scope).
    if (!callee->isImplicit() && !mgr.isInMainFile(callee->getLocation()) &&
        mgr.isInSystemHeader(callee->getLocation()))
      return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);
  }

  clang::QualType returnType = E->getCallReturnType(*_C);
  // A null return type can come back for calls whose callee type can't be
  // resolved to a FunctionType (e.g. macro-expanded)
  if (returnType.isNull())
    return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);
  if (returnType->isVoidType()) {
    // A void call yields no value to havoc; drop it (the statement's
    // semicolon stays behind, leaving an empty statement)
    _Rewriter.ReplaceText(E->getSourceRange(), "");
  } else if (std::optional<std::string> suffix = verifierSuffixForType(returnType)) {
    _Rewriter.ReplaceText(E->getSourceRange(), "__VERIFIER_nondet_" + *suffix + "()");
    _NeededSuffixes->emplace(*suffix);
  } else if (returnType->isAnyPointerType() && !returnType->isFunctionPointerType()) {
    // Pointer returns get a havocked-but-valid block (SV-COMP
    // __VERIFIER_nondet_memory semantics: dereferencing an arbitrary nondet
    // pointer value is undefined, so the pointer itself must be real).
    // Block size is a fixed guess for now; char pointers are
    // null-terminated so string ops stay in bounds. AddVerifiersConsumer
    // emits the helper definitions when it sees these markers.
    bool isCharPtr = returnType->getPointeeType()->isAnyCharacterType();
    std::string helper = isCharPtr ? "__havoc_cstring" : "__havoc_block";
    _Rewriter.ReplaceText(E->getSourceRange(), helper + "(128)");
    _NeededSuffixes->emplace(helper);
  }
  // Aggregate returns (structs, unions) have no expression-position nondet
  // equivalent; those calls are left as-is
  return clang::RecursiveASTVisitor<HavocCallsVisitor>::VisitCallExpr(E);
}

bool HavocCallsVisitor::shouldTraversePostOrder() { return true; }
