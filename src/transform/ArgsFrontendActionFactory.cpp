#include "ArgsFrontendActionFactory.hpp"
#include <GenerateIncludeAction.hpp>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

// Constructor for our front end that allows for the output stream variable to
// be passed to all consumers and actions to come
ArgsFrontendFactory::ArgsFrontendFactory(llvm::raw_fd_ostream &output)
    : _Output(output) {}

// overridden create method necessary to out put our frontend action
std::unique_ptr<clang::FrontendAction> ArgsFrontendFactory::create() {
  return std::make_unique<GenerateIncludeAction>(_Output);
}
