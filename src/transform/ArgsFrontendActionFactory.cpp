#include "ArgsFrontendActionFactory.hpp"
#include "TransformAction.hpp"

#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

ArgsFrontendFactory::ArgsFrontendFactory(llvm::raw_ostream &output) : _Output(output) {}

std::unique_ptr<clang::FrontendAction> ArgsFrontendFactory::create() {
  return std::make_unique<TransformAction>(_Output);
}

bool ArgsFrontendFactory::runInvocation(
    std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files,
    std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps,
    clang::DiagnosticConsumer *DiagConsumer) {
  return clang::tooling::FrontendActionFactory::runInvocation(Invocation, Files, PCHContainerOps,
                                                              DiagConsumer);
}
