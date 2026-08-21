// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "MainGenConsumer.hpp"

#include "DebugLog.hpp"
#include "HarnessHeaderData.hpp"
#include "VerifierNames.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <optional>
#include <string>
#include <vector>

MainGenConsumer::MainGenConsumer(std::shared_ptr<std::set<std::string>> noOpFunctions,
                                 clang::Rewriter &rewriter, const HavocBounds &havoc)
    : _NoOpFunctions(noOpFunctions), _Rewriter(rewriter), _Havoc(havoc) {}

void MainGenConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::SourceManager &mgr = Context.getSourceManager();

  // The generated harness always emits nondet calls, so the header is included
  // unconditionally. Its __HAVOC_* bounds are emitted here rather than in the
  // header itself: the header is one file shared by every benchmark and is
  // rewritten each run, so per-benchmark bounds must live in the .c to survive
  // hand-editing.
  std::string prelude = "#define __HAVOC_ARGC_MIN " + std::to_string(_Havoc.argcMin) + "\n" +
                        "#define __HAVOC_ARGC_MAX " + std::to_string(_Havoc.argcMax) + "\n" +
                        "#define __HAVOC_STR_MAX " + std::to_string(_Havoc.strMax) + "\n" +
                        "#define __HAVOC_BLOCK_MAX " + std::to_string(_Havoc.blockMax) + "\n" +
                        "#include \"" + std::string(kArgvCHarnessHeaderName) + "\"\n";
  _Rewriter.InsertTextBefore(mgr.translateLineCol(mgr.getMainFileID(), 1, 1), prelude + "\n");

  // Rename any existing main so the generated one is the sole entry point;
  // original_main is then harnessed like any other function.
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
    // argc/argv is a known pointer contract, so synthesize a real call rather
    // than skipping it as an unsupported-param function.
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
    if (!supported) {
      debugLog(2, "Warning: only primitive symbolics supported; " + func->getNameAsString() +
                      " not harnessed (filter's param check did not strip it)");
      continue;
    }
    harness += "  " + func->getNameAsString() + "(" + args + ");\n";
  }

  std::string mainFn = "\nint main(void) {\n" + harness + "  return 0;\n}\n";
  _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainFn);
}

std::string MainGenConsumer::genMainHarness(const clang::FunctionDecl *mainFn) {
  unsigned numParams = mainFn->getNumParams();

  if (numParams == 0)
    return "  original_main();\n";

  // int main(int argc, char **argv[, char **envp]): argc and a matching argv
  // are synthesized entirely by the argv_c_harness.h helpers.
  std::string body;
  body += "  int argc = __HAVOC_ARGC();\n";
  body += "  original_main(argc, __havoc_argv_fill(argc));\n";
  return body;
}
