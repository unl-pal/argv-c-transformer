#include "include/Transformer.hpp"
// #include "CodeGeneratorAction.hpp"
#include "ArgsFrontendActionFactory.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInvocation.h>
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
#include <string>
#include <vector>

// Take an individual file and apply all transformations to it by generating 
// the ast, visitors and regenerating the source code as precompiled .i file
// returns false if the AST fails to build
bool Transformer::transformFile(std::filesystem::path path,
                             std::vector<std::string> &args) {
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

  std::vector<std::string> sources;
  sources.push_back(path);

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
    return 1;
  }

  std::cout << "Setting Up Common Options Parser" << std::endl;

  llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser = clang::tooling::CommonOptionsParser::create(argc, (const char**)argv, myToolCategory);

  if (!expectedParser) {
    llvm::errs() << expectedParser.takeError();
    return 1;
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

  return 0;
}

// Recursive algorithm for traversing the file structure and searching for 
// relavent c files to transform
//     ideally files will have been filtered but some logic exists to prevent
//     mishaps just incase
// Returns false if any C files failed transformation
bool Transformer::transformAll(std::filesystem::path path,
                            std::vector<std::string> &args) {
  if (std::filesystem::exists(path)) {
    if (std::filesystem::is_directory(path)) {
      for (const std::filesystem::directory_entry &entry :
           std::filesystem::directory_iterator(path)) {
        /*std::cout << "Dir " << std::endl;*/
        transformAll(entry.path(), args);
      }
    } else if (std::filesystem::is_regular_file(path)) {
      if (path.has_extension() && path.extension() == ".c") {
        /*std::cout << "File " << std::endl;*/
        return transformFile(path, args);
      }
    }
  }
  return true;
}

void Transformer::parseConfig() {
}

// Main function should be transfered to a driver for use via the full implementation
int Transformer::run(std::string filePath) {
  std::filesystem::path path(filePath);
  if (std::filesystem::exists(path)) {
    parseConfig();
    // Set args for AST creation
    std::vector<std::string> args = std::vector<std::string>();
    args.push_back("-fparse-all-comments");
    args.push_back("-resource-dir=" + std::string(std::getenv("CLANG_RESOURCES")));
    // run the transformer on the file structure
    if (transformAll(path, args)) {
      return 0;
    }
  }
  return 1;
}
