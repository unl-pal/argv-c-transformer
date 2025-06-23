#include "FrontendFactoryWithArgs.hpp"
#include "FilterAction.hpp"

#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

// Constructor for our front end that allows for the output stream variable to
// be passed to all consumers and actions to come
FrontendFactoryWithArgs::FrontendFactoryWithArgs(llvm::raw_fd_ostream &output)
    : _Output(output) {}

// overridden create method necessary to out put our frontend action
std::unique_ptr<clang::FrontendAction> FrontendFactoryWithArgs::create() {
  return std::make_unique<FilterAction>(_Output);
}

bool FrontendFactoryWithArgs::runInvocation(std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files, std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps, clang::DiagnosticConsumer *DiagConsumer) {
  llvm::outs() << "Running Invocation\n";

  // return true;
  return clang::tooling::FrontendActionFactory::runInvocation(Invocation, Files, PCHContainerOps, DiagConsumer);
}
