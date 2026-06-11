#include "MainGenConsumer.hpp"

#include "VerifierNames.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <optional>
#include <string>
#include <vector>

MainGenConsumer::MainGenConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                                 clang::Rewriter &rewriter)
    : _NeededSuffixes(neededSuffixes), _Rewriter(rewriter) {}

void MainGenConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::SourceManager &mgr = Context.getSourceManager();

  // Collect every function defined in this file, in source order, renaming
  // any existing main along the way so the generated main below is the sole
  // entry point. The renamed original_main is then harnessed like any other
  // function. Calls to main from within the file are not rewritten (calling
  // main is vanishingly rare in C and ill-formed in most dialects).
  std::vector<const clang::FunctionDecl *> defined;
  for (clang::Decl *decl : Context.getTranslationUnitDecl()->decls()) {
    const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (!func || !mgr.isInMainFile(func->getLocation()))
      continue;
    if (func->isMain())
      _Rewriter.ReplaceText(func->getNameInfo().getSourceRange(), "original_main");
    if (func->isThisDeclarationADefinition())
      defined.push_back(func);
  }

  std::string harness;
  for (const clang::FunctionDecl *func : defined) {
    if (func->isVariadic())
      continue;
    std::string args;
    std::set<std::string> argSuffixes;
    bool supported = true;
    for (const clang::ParmVarDecl *parm : func->parameters()) {
      std::optional<std::string> suffix = verifierSuffixForType(parm->getOriginalType());
      if (!suffix) {
        supported = false;
        break;
      }
      if (!args.empty())
        args += ", ";
      args += "__VERIFIER_nondet_" + *suffix + "()";
      argSuffixes.insert(*suffix);
    }
    // A parameter without a nondet equivalent (pointer, struct, ...): skip
    // this function but keep harnessing the rest.
    if (!supported)
      continue;
    std::string name = func->isMain() ? "original_main" : func->getNameAsString();
    harness += "  " + name + "(" + args + ");\n";
    _NeededSuffixes->insert(argSuffixes.begin(), argSuffixes.end());
  }

  std::string mainFn = "\nint main(void) {\n" + harness + "  return 0;\n}\n";
  _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainFn);
}
