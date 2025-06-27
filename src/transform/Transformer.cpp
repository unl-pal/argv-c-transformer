#include "include/Transformer.hpp"
#include "ArgsFrontendActionFactory.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Serialization/PCHContainerOperations.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <cstring>
#include <filesystem>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringRef.h>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>
#include <vector>

// Take an individual file and apply all transformations to it by generating 
// the ast, visitors and regenerating the source code as precompiled .i file
// returns false if the AST fails to build
bool Transformer::transformFile(std::filesystem::path path) {
  std::cout << "Transforming: " << path.string() << std::endl;
  if (!std::filesystem::exists(path)) return false;
  std::filesystem::path full = std::filesystem::current_path() / path;

  std::filesystem::path srcPath = std::filesystem::path("benchmark");
  std::filesystem::path preprocessedPath = std::filesystem::path("preprocessed");
  // Keeps people from being able to write files outside of the project folder for now
  // Eliminates the filteredFiles part of the path
  for (const std::filesystem::path &component : path) {
    if (component.string() != "filteredFiles" && component.string() != "..") {
      srcPath /= component;
    }
  }

  std::cout << "Creating resources for Transformation" << std::endl;

  std::error_code ec;

  std::filesystem::create_directories(srcPath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(srcPath.string()), ec);

  static llvm::cl::OptionCategory myToolCategory("transformer");

  clang::IgnoringDiagConsumer diagConsumer;

  std::string resourceDir = std::getenv("CLANG_RESOURCES");

  std::cout << "Setting Comp Options" << std::endl;

  std::vector<std::string> compOptionsArgs({
    "clang",
    path.string(),
    // "--",
    "-extra-arg=-fparse-all-comments",
    "-extra-arg=-resource-dir=" + resourceDir,
    "-extra-arg=-xc",
    "-extra-arg=-I"
  });

  int argc = compOptionsArgs.size();

  char** argv = new char*[argc + 1];

  for (int i=0; i<argc; i++) {
    argv[i] = new char[compOptionsArgs[i].length() + 1];
    std::strcpy(argv[i], compOptionsArgs[i].c_str());
  }

  argv[argc] = nullptr;

  if (argv == nullptr) {
    return 0;
  }

  std::cout << "Setting Up Common Options Parser" << std::endl;

  llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser = clang::tooling::CommonOptionsParser::create(argc, (const char**)argv, myToolCategory);

  if (!expectedParser) {
    llvm::errs() << expectedParser.takeError();
    return 0;
  }

  clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();

  std::cout << "Building the Tool" << std::endl;

  clang::tooling::ClangTool tool(optionsParser.getCompilations(),
                                 optionsParser.getSourcePathList());

  std::cout << "Diagnostic Options" << std::endl;

  tool.setDiagnosticConsumer(&diagConsumer);

  std::cout << "Factory" << std::endl;

  ArgsFrontendFactory factory(output);

  std::cout << "Run the Tool" << std::endl;

  llvm::outs() << tool.run(&factory) << "\n";

  output.close();

  if (!checkCompilable(path)) { // && !configs->keepCompilesOnly) {
    // std::filesystem::remove(path);
    return 0;
  }

  return 1;
}

// Recursive algorithm for traversing the file structure and searching for 
// relavent c files to transform
//     ideally files will have been filtered but some logic exists to prevent
//     mishaps just incase
// Returns false if any C files failed transformation
int Transformer::transformAll(std::filesystem::path path, int count) {
  if (std::filesystem::exists(path)) {
    if (std::filesystem::is_directory(path)) {
      int successes = 0;
      for (const std::filesystem::directory_entry &entry :
           std::filesystem::directory_iterator(path)) {
        successes += transformAll(entry.path(), count);
      }
      return count + successes;
    } else if (std::filesystem::is_regular_file(path)) {
      if (path.has_extension() && path.extension() == ".c") {
        return count + transformFile(path);
      }
    }
  }
  return count;
}

int Transformer::checkCompilable(std::filesystem::path path) {
  static llvm::cl::OptionCategory myToolCategory("CheckCompiles");

  std::string resourceDir = std::getenv("CLANG_RESOURCES");

  std::vector<std::string> compOptionsArgs({
    "clang",
    path.string(),
    "verifier.c",
    "-extra-arg=-xc",
    "-extra-arg=-I",
    "-extra-arg=-w",
    // "-extra-arg=-fsyntax-only",
    // "-extra-arg=-fparse-all-comments",
    "-extra-arg=-resource-dir=" + resourceDir,
  });

  int argc = compOptionsArgs.size();

  char** argv = new char*[argc + 1];

  for (int i=0; i<argc; i++) {
    argv[i] = new char[compOptionsArgs[i].length() + 1];
    std::strcpy(argv[i], compOptionsArgs[i].c_str());
  }

  argv[argc] = nullptr;

  if (argv == nullptr) {
    return 0;
  }

  llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser = clang::tooling::CommonOptionsParser::create(argc, (const char**)argv, myToolCategory);

  if (!expectedParser) {
    llvm::errs() << expectedParser.takeError();
    return 0;
  }

  clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();

  clang::tooling::ClangTool tool(optionsParser.getCompilations(),
                                 optionsParser.getSourcePathList());

  // Show the number of errors only not the errors themselves to avoid clutter
  clang::DiagnosticConsumer diagConsumer;
  tool.setDiagnosticConsumer(&diagConsumer);

  tool.run(clang::tooling::newFrontendActionFactory<clang::PreprocessOnlyAction>().get());

  // If there are errors do not count the file as compilable
  if (diagConsumer.getNumErrors()) {
    return 0;
  }
  return 1;
}

void Transformer::parseConfig() {
}

// Main function should be transfered to a driver for use via the full implementation
int Transformer::run(std::string filePath) {
  std::filesystem::path path(filePath);
  if (std::filesystem::exists(path)) {
    parseConfig();
    // run the transformer on the file structure
    int result = transformAll(path, 0);
    std::cout << "Number of Compilable Benchmarks: " << result << std::endl;
    return result;
  }
  return 0;
}
