#include "MainGenConsumer.hpp"

#include "VerifierNames.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <optional>
#include <string>
#include <vector>
#include <iostream>

MainGenConsumer::MainGenConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                                 clang::Rewriter &rewriter)
    : _NeededSuffixes(neededSuffixes), _Rewriter(rewriter) {}

void MainGenConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::SourceManager &mgr = Context.getSourceManager();

  // Collect every function defined in this file, in source order, renaming
  // any existing main along the way so the generated main below is the sole
  // entry point. The renamed original_main is then harnessed like any other
  // function
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
    // main has a known pointer contract (argc/argv), so synthesize a real
    // call instead of skipping it like an arbitrary unsupported-param function
    if (func->isMain()) {
      harness += genMainHarness(func);
      continue;
    }
    if (func->isVariadic()) {
      std::cout << "Warning: variadic functions unsupported\n" + func->getNameAsString() +
                       " not harnessed"
                << std::endl;
      continue;
    }
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
    if (!supported) {
      std::cout << "Warning: only primitive symbolics supported\n" + func->getNameAsString() +
                       " not harnessed"
                << std::endl;
      std::cerr << "This function should not have survived the filter step" << std::endl;
      continue;
    }
    std::string name = func->isMain() ? "original_main" : func->getNameAsString();
    harness += "  " + name + "(" + args + ");\n";
    _NeededSuffixes->insert(argSuffixes.begin(), argSuffixes.end());
  }

  std::string mainFn = "\nint main(void) {\n" + harness + "  return 0;\n}\n";
  _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainFn);
}

std::string MainGenConsumer::genMainHarness(const clang::FunctionDecl *mainFn) {
  unsigned numParams = mainFn->getNumParams();

  // int main(void): no args to synthesize, just call it.
  if (numParams == 0)
    return "  original_main();\n";

  // int main(int argc, char **argv[, char **envp]): build a nondet argc and
  // an argv of havocked C strings, then call original_main(argc, argv).

  std::string body;
  body += "  extern void abort(void);\n";
  body += "  int argc = __VERIFIER_nondet_int();\n";
  body += "  if (argc < 0 || argc > 7) abort();\n";
  body += "  char *argv[argc + 1];\n";
  body += "  for (int i = 0; i < argc; i++)\n";
  body += "    argv[i] = __havoc_cstring(16);\n";
  body += "  argv[argc] = 0;\n";
  body += "  original_main(argc, argv);\n";

  _NeededSuffixes->insert("int");
  _NeededSuffixes->insert("__havoc_cstring");
  return body;
}
