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
      debugLog(2, "[transform] " + func->getNameAsString() + " body collapsed to no-ops; not harnessed");
      continue;
    }
    // argc/argv is a known pointer contract, so synthesize a real call rather
    // than skipping it as an unsupported-param function.
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
    // A parameter with neither a nondet equivalent nor a viable pointer plan
    // (aggregate by value, function pointer, ...): skip this function but keep
    // harnessing the rest.
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

  // The generated harness always emits nondet calls, so the header is included
  // unconditionally. Its __HAVOC_* bounds are emitted here rather than in the
  // header itself: the header is one file shared by every benchmark and is
  // rewritten each run, so per-benchmark bounds must live in the .c to survive
  // hand-editing.
  std::string prelude = "#define __HAVOC_ARGC_MIN " + std::to_string(_Havoc.argcMin) + "\n" +
                        "#define __HAVOC_ARGC_MAX " + std::to_string(_Havoc.argcMax) + "\n" +
                        "#define __HAVOC_STR_MAX " + std::to_string(_Havoc.strMax) + "\n" +
                        "#define __HAVOC_BLOCK_MAX " + std::to_string(_Havoc.blockMax) + "\n" +
                        "#define __HAVOC_ARRAY_ELEMS " + std::to_string(_Havoc.arrayElems) + "\n" +
                        "#include \"" + std::string(kArgvCHarnessHeaderName) + "\"\n";

  // A struct named only inside a harnessed function's parameter list has
  // prototype scope; hoist it to file scope, ahead of every function that
  // might reference it - including its own prototype-scope declaration, since
  // two declarations of "the same" tag at different scopes are actually
  // distinct, incompatible types. HavocCallsVisitor's pointer-return havocking
  // shares this set, so it is only complete once the harness loop above (which
  // calls genCallHarness, filling in the parameter side) has finished.
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

  // Classify every parameter first: a pointer anywhere in the list changes how
  // the integer parameters are synthesized, so nothing can be emitted until the
  // whole list has been seen.
  std::vector<PointerPlan> plans;
  bool anyPointer = false;
  for (const clang::ParmVarDecl *parm : func->parameters()) {
    // getOriginalType, not getType: a parameter spelled T[N] has already
    // decayed to T* in the latter, discarding the bound planPointer wants.
    PointerPlan plan;
    if (!verifierSuffixForType(parm->getOriginalType())) {
      plan = planPointer(parm->getOriginalType(), mgr);
      if (!plan.viable)
        return call; // viable stays false; caller skips this function
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
      // Parameters are in statement position, so the pointer is havocked as
      // stack storage rather than a heap block: no free obligation, no
      // allocator to model. The storage decays to the parameter's type at the
      // call, so no cast is needed except on the opaque byte buffer.
      std::string local = "__h" + std::to_string(counter++);
      PointerStorage store = renderPointerStorage(plans[i], declared, local,
                                                  parm->getType().getAsString(),
                                                  Context.getPrintingPolicy());
      call.prologue += store.decls;
      call.args += store.arg;
      if (!plans[i].fwdDecl.empty())
        _NeededFwdDecls->insert(plans[i].fwdDecl);
      continue;
    }

    // With a pointer in the list, any integer is a candidate index into it.
    // Clamping every integer to the block's element count is always safe;
    // inferring which parameter is "the length" from its name is not, since a
    // wrong guess sizes the block too small and invents an out-of-bounds.
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

  // int main(int argc, char **argv[, char **envp]): argc and a matching argv
  // are synthesized entirely by the argv_c_harness.h helpers.
  std::string body;
  body += "  int argc = __HAVOC_ARGC();\n";
  body += "  original_main(argc, __havoc_argv_fill(argc));\n";
  return body;
}
