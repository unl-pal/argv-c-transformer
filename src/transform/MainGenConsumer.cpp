// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "MainGenConsumer.hpp"

#include "DebugLog.hpp"
#include "HarnessHeaderData.hpp"
#include "HavocPolicy.hpp"
#include "VerifierNames.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <optional>
#include <string>
#include <vector>

MainGenConsumer::MainGenConsumer(std::shared_ptr<std::set<std::string>> noOpFunctions,
                                 std::shared_ptr<std::set<std::string>> neededFwdDecls,
                                 clang::Rewriter &rewriter, const HavocBounds &havoc)
    : _NoOpFunctions(noOpFunctions), _NeededFwdDecls(neededFwdDecls), _Rewriter(rewriter),
      _Havoc(havoc) {}

void MainGenConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::SourceManager &mgr = Context.getSourceManager();

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
      debugLog(2, "[transform] " + func->getNameAsString() + " body collapsed to no-ops; not harnessed");
      continue;
    }
    if (func->isMain()) {
      harness += genMainHarness(func);
      continue;
    }
    if (func->isVariadic()) {
      debugLog(2, "[transform] variadic functions unsupported; " + func->getNameAsString() +
                      " not harnessed");
      continue;
    }
    HarnessCall call = genCallHarness(func, Context);
    if (!call.viable) {
      debugLog(2, "[transform] unsupported parameter type; " + func->getNameAsString() +
                      " not harnessed");
      continue;
    }
    harness += call.prologue;
    harness += "  " + func->getNameAsString() + "(" + call.args + ");\n";
  }

  std::string mainFn = "\nint main(void) {\n" + harness + "  return 0;\n}\n";
  _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainFn);

  // emitted here, not the shared header, so bounds survive hand-editing of the .c
  std::string prelude = "#define __HAVOC_ARGC_MIN " + std::to_string(_Havoc.argcMin) + "\n" +
                        "#define __HAVOC_ARGC_MAX " + std::to_string(_Havoc.argcMax) + "\n" +
                        "#define __HAVOC_STR_MAX " + std::to_string(_Havoc.strMax) + "\n" +
                        "#define __HAVOC_BLOCK_MAX " + std::to_string(_Havoc.blockMax) + "\n" +
                        "#define __HAVOC_ARRAY_ELEMS " + std::to_string(_Havoc.arrayElems) + "\n" +
                        "#include \"" + std::string(kArgvCHarnessHeaderName) + "\"\n";

  // also populated by HavocCallsVisitor; must run after both
  std::string fwdDecls;
  for (const std::string &decl : *_NeededFwdDecls)
    fwdDecls += decl + ";\n";
  if (!fwdDecls.empty())
    prelude += fwdDecls;

  _Rewriter.InsertTextBefore(mgr.translateLineCol(mgr.getMainFileID(), 1, 1), prelude + "\n");
}

MainGenConsumer::HarnessCall
MainGenConsumer::genCallHarness(const clang::FunctionDecl *func, clang::ASTContext &Context) {
  const clang::SourceManager &mgr = Context.getSourceManager();
  HarnessCall call;

  // classified up front: a pointer in the list changes integer synthesis below
  std::vector<PointerPlan> plans;
  bool anyPointer = false;
  for (const clang::ParmVarDecl *parm : func->parameters()) {
    PointerPlan plan;
    if (!verifierSuffixForType(parm->getOriginalType())) {
      plan = planPointer(parm->getOriginalType(), mgr);
      if (!plan.viable)
        return call;
      anyPointer = true;
    }
    plans.push_back(plan);
  }

  unsigned counter = _LocalCounter;
  for (size_t i = 0; i < plans.size(); ++i) {
    const clang::ParmVarDecl *parm = func->parameters()[i];
    clang::QualType declared = parm->getOriginalType();
    if (!call.args.empty())
      call.args += ", ";

    std::optional<std::string> suffix = verifierSuffixForType(declared);
    if (!suffix) {
      std::string local = "__h" + std::to_string(counter++);
      PointerStorage store = renderPointerStorage(plans[i], declared, local,
                                                  parm->getType().getAsString());
      call.prologue += store.decls;
      call.args += store.arg;
      if (!plans[i].fwdDecl.empty())
        _NeededFwdDecls->insert(plans[i].fwdDecl);
      continue;
    }

    // clamp every integer rather than guessing which one is "the length"
    if (anyPointer && declared->isIntegerType() && !declared->isBooleanType()) {
      std::string local = "__h" + std::to_string(counter++);
      call.prologue += "  " + declared.getUnqualifiedType().getAsString() + " " + local +
                       " = __VERIFIER_nondet_" + *suffix + "();\n  if (";
      if (declared->isSignedIntegerType())
        call.prologue += local + " < 0 || ";
      call.prologue += local + " > __HAVOC_ARRAY_ELEMS) abort();\n";
      call.args += local;
      continue;
    }
    call.args += "__VERIFIER_nondet_" + *suffix + "()";
  }

  _LocalCounter = counter;
  call.viable = true;
  return call;
}

std::string MainGenConsumer::genMainHarness(const clang::FunctionDecl *mainFn) {
  unsigned numParams = mainFn->getNumParams();

  if (numParams == 0)
    return "  original_main();\n";

  std::string body;
  body += "  int argc = __HAVOC_ARGC();\n";
  body += "  original_main(argc, __havoc_argv_fill(argc));\n";
  return body;
}
