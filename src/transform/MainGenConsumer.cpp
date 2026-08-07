// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "MainGenConsumer.hpp"

#include "DebugLog.hpp"
#include "HavocPolicy.hpp"
#include "VerifierNames.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <optional>
#include <string>
#include <vector>

MainGenConsumer::MainGenConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                                 std::shared_ptr<std::set<std::string>> noOpFunctions,
                                 clang::Rewriter &rewriter)
    : _NeededSuffixes(neededSuffixes), _NoOpFunctions(noOpFunctions), _Rewriter(rewriter) {}

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
    if (_NoOpFunctions->count(func->getNameAsString())) {
      debugLog(2, "[transform] " + func->getNameAsString() + " body collapsed to no-ops; not harnessed");
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
    std::string name = func->isMain() ? "original_main" : func->getNameAsString();
    harness += call.prologue;
    harness += "  " + name + "(" + call.args + ");\n";
  }

  std::string mainFn = "\nint main(void) {\n" + harness + "  return 0;\n}\n";
  _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainFn);
}

MainGenConsumer::HarnessCall
MainGenConsumer::genCallHarness(const clang::FunctionDecl *func, clang::ASTContext &Context) {
  const clang::SourceManager &mgr = Context.getSourceManager();
  HarnessCall call;
  // Markers stay local until the whole list is known to be harnessable, so a
  // function that turns out to be unsupported leaves no unused externs behind.
  std::set<std::string> markers;

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
      markers.insert("__havoc_mem");
      if (store.cstring)
        markers.insert("__havoc_str");
      markers.insert("__havoc_bounds");
      if (!plans[i].fwdDecl.empty())
        markers.insert("__havoc_fwd:" + plans[i].fwdDecl);
      continue;
    }

    markers.insert(*suffix);
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
      markers.insert("__havoc_bounds");
      continue;
    }
    call.args += "__VERIFIER_nondet_" + *suffix + "()";
  }

  _LocalCounter = counter;
  _NeededSuffixes->insert(markers.begin(), markers.end());
  call.viable = true;
  return call;
}

std::string MainGenConsumer::genMainHarness(const clang::FunctionDecl *mainFn) {
  unsigned numParams = mainFn->getNumParams();

  // int main(void): no args to synthesize, just call it.
  if (numParams == 0)
    return "  original_main();\n";

  // int main(int argc, char **argv[, char **envp]): build a nondet argc and
  // an argv of havocked C strings, then call original_main(argc, argv).
  // The bounds are __HAVOC_* macros emitted by AddVerifiersConsumer, so the
  // generated benchmark stays retunable by hand.

  std::string body;
  body += "  int argc = __VERIFIER_nondet_int();\n";
  body += "  if (argc < __HAVOC_ARGC_MIN || argc > __HAVOC_ARGC_MAX) abort();\n";
  // Backing storage for the strings lives in main's frame, so every argv[i]
  // stays valid until the program exits - no per-argument heap allocation to
  // leak. Each row is nondet-filled and given an in-bounds terminator, the same
  // string invariant renderPointerStorage plants for a char* parameter.
  body += "  char __argv_buf[__HAVOC_ARGC_MAX][__HAVOC_STR_MAX];\n";
  body += "  char *argv[__HAVOC_ARGC_MAX + 1];\n";
  body += "  for (int i = 0; i < argc; i++) {\n";
  body += "    __VERIFIER_nondet_memory(__argv_buf[i], __HAVOC_STR_MAX);\n";
  body += "    size_t len = __VERIFIER_nondet_size_t();\n";
  body += "    if (len >= __HAVOC_STR_MAX) abort();\n";
  body += "    __argv_buf[i][len] = '\\0';\n";
  body += "    argv[i] = __argv_buf[i];\n";
  body += "  }\n";
  body += "  argv[argc] = 0;\n";
  body += "  original_main(argc, argv);\n";

  _NeededSuffixes->insert("int");
  _NeededSuffixes->insert("__havoc_mem");
  _NeededSuffixes->insert("__havoc_str");
  _NeededSuffixes->insert("__havoc_argv");
  return body;
}
