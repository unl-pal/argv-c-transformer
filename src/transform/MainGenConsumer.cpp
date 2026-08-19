// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "MainGenConsumer.hpp"

#include "DebugLog.hpp"
#include "RuntimeHeaderData.hpp"
#include "VerifierNames.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <optional>
#include <string>
#include <vector>

MainGenConsumer::MainGenConsumer(std::shared_ptr<std::set<std::string>> noOpFunctions,
                                 clang::Rewriter &rewriter)
    : _NoOpFunctions(noOpFunctions), _Rewriter(rewriter) {}

void MainGenConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::SourceManager &mgr = Context.getSourceManager();

  // Every transformed file uses at least one __VERIFIER_nondet_* call (this
  // consumer's own harness guarantees that), so the runtime header - which
  // unconditionally declares all of them plus the __HAVOC_*/reach_error
  // scaffolding - is always needed.
  std::string prelude = "#include \"" + std::string(kArgvCRuntimeHeaderName) + "\"\n";
  _Rewriter.InsertTextBefore(mgr.translateLineCol(mgr.getMainFileID(), 1, 1), prelude + "\n");

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
    if (_NoOpFunctions->count(func->getNameAsString())) {
      debugLog(2, "Info: " + func->getNameAsString() + " body collapsed to no-ops; not harnessed");
      continue;
    }
    // main has a known pointer contract (argc/argv), so synthesize a real
    // call instead of skipping it like an arbitrary unsupported-param function
    // (note no-op supercedes this)
    if (func->isMain()) {
      harness += genMainHarness(func);
      continue;
    }
    if (func->isVariadic()) {
      debugLog(2, "Warning: variadic functions unsupported; " + func->getNameAsString() +
                      " not harnessed");
      continue;
    }
    std::string args;
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
    }
    // A parameter without a nondet equivalent (pointer, struct, ...): skip
    // this function but keep harnessing the rest.
    if (!supported) {
      debugLog(2, "Warning: only primitive symbolics supported; " + func->getNameAsString() +
                      " not harnessed (filter's param check did not strip it)");
      continue;
    }
    std::string name = func->isMain() ? "original_main" : func->getNameAsString();
    harness += "  " + name + "(" + args + ");\n";
  }

  std::string mainFn = "\nint main(void) {\n" + harness + "  return 0;\n}\n";
  _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainFn);
}

std::string MainGenConsumer::genMainHarness(const clang::FunctionDecl *mainFn) {
  unsigned numParams = mainFn->getNumParams();

  // int main(void): no args to synthesize, just call it.
  if (numParams == 0)
    return "  original_main();\n";

  // int main(int argc, char **argv[, char **envp]): a nondet, bounded argc
  // and a matching havocked argv, both synthesized entirely by
  // __HAVOC_ARGC()/__HAVOC_ARGV() (argv_c_runtime.h) - the __HAVOC_* bounds
  // are macros in that same header, so the generated benchmark stays
  // retunable by hand.
  std::string body;
  body += "  int argc = __HAVOC_ARGC();\n";
  body += "  original_main(argc, __HAVOC_ARGV(argc));\n";
  return body;
}
