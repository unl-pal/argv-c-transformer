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
#include <fstream>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringRef.h>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <regex>
#include <string>
#include <system_error>
#include <vector>

const int defaultDebugLevel = 0;
const bool defaultKeepCompilesOnly = true;
const std::string defaultFilterDir = "filteredFiles";
const std::string defaultBenchmarkDir = "benchmarks";
/// Not yet implemented in code - currently handled by scripts
const bool defaultWipeOldBenchmarks = true;

Transformer::Transformer(std::string configFile) : configuration() {
  /// Set default values - figure out if this
  configuration.debugLevel = defaultDebugLevel;
  configuration.keepCompilesOnly = defaultKeepCompilesOnly;
  configuration.filterDir = defaultFilterDir;
  configuration.benchmarkDir = defaultBenchmarkDir;
  configuration.wipeOldBenchmarks = defaultWipeOldBenchmarks;
  parseConfig(configFile);
}

// Take an individual file and apply all transformations to it by generating 
// the ast, visitors and regenerating the source code as precompiled .i file
// returns false if the AST fails to build
bool Transformer::transformFile(std::filesystem::path path) {
  std::cout << "Transforming: " << path.string() << std::endl;
  if (!std::filesystem::exists(path)) return false;
  std::filesystem::path full = std::filesystem::current_path() / path;

  std::filesystem::path srcPath = std::filesystem::path(configuration.benchmarkDir);
  /// May want this later for .i files that could be used for expanding macros
  // std::filesystem::path preprocessedPath = std::filesystem::path("preprocessed");

  // Keeps people from being able to write files outside of the project folder for now
  // Eliminates the filteredFiles part of the path
  for (const std::filesystem::path &component : path) {
    if (component.string() != configuration.filterDir && component.string() != "..") {
      srcPath /= component;
    }
  }

  std::cout << "Creating resources for Transformation" << std::endl;

  static llvm::cl::OptionCategory myToolCategory("transformer");

  clang::IgnoringDiagConsumer diagConsumer;

  std::string resourceDir;
  try {
    resourceDir = std::getenv("CLANG_RESOURCES");
  } catch (...) {
    std::cout << "Please set the CLANG_RESOURCES environment vairable "
      "before proceeding"
      << std::endl;
    return 1;
  }

  std::cout << "Setting Comp Options" << std::endl;

  std::vector<std::string> compOptionsArgs({
    "clang",
    "-extra-arg=-xc",
    "-extra-arg=-I",
    path.string(),
    "-extra-arg=-resource-dir=" + resourceDir,
    "-extra-arg=-fparse-all-comments",
    // "--",
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

  std::error_code ec;

  std::filesystem::create_directories(srcPath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(srcPath.string()), ec);

  ArgsFrontendFactory factory(output);

  std::cout << "Run the Tool" << std::endl;

  llvm::outs() << tool.run(&factory) << "\n";

  output.close();

  if (!checkCompilable(srcPath)) {
    if (configuration.keepCompilesOnly) {
      std::filesystem::remove(srcPath);
    }
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
    "-extra-arg=-fsyntax-only",
    "-extra-arg=-xc",
    "-extra-arg=-I",
    "-extra-arg=-resource-dir=" + resourceDir,
    "verifier.c",
    path.string(),
    // "-extra-arg=-w",
    // "-extra-arg=-fparse-all-comments",
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

  // tool.run(clang::tooling::newFrontendActionFactory<clang::PreprocessOnlyAction>().get());
  tool.run(clang::tooling::newFrontendActionFactory<clang::SyntaxOnlyAction>().get());

  // If there are errors do not count the file as compilable
  if (diagConsumer.getNumErrors()) {
    return 0;
  }
  return 1;
}

/// TODO MAKE THE PARSERS MORE SECURE!!
void Transformer::parseConfig(std::string configFile) {
  std::ifstream file(configFile);
  if (!std::filesystem::exists(configFile)) {
    std::cout << "File: " << configFile << " Does Not Exist" << std::endl;
    std::cout << "Using Default Settings" << std::endl;
    return;
  }
  if (file.is_open()) {
    std::cout << "Using: " << configFile << " Specified Settings" << std::endl;
    std::regex pattern("^\\s*(\\w+)\\s*=\\s*([0-9]+|[\\w\\s,]+|[\\w/-_.]+)$");
    std::string line;
    std::smatch match;
    while (std::getline(file, line)) {
      if (std::regex_search(line, match, pattern)) {
        std::string key = match[1];
        std::string value = match[2];
        // Add the value to the config if the key is a member of the map
        if (key == "benchmarkDir") {
          configuration.benchmarkDir = value;
          if (!std::filesystem::exists(value)) {
            std::filesystem::create_directory(value);
          }
        } else if (key == "filterDir") {
          configuration.filterDir = value;
          if (!std::filesystem::exists(value)) {
            std::cout << "There is no directory: " << value
                      << " to use as Filter Directory\n"
                      << "Aborting Filtering Attempt"
                      << std::endl;
          }
        } else if (key == "debugLevel") {
          try {
            configuration.debugLevel = std::stoi(value);
          } catch (...) {
            configuration.debugLevel = 0;
          }
        } else if (key == "keepCompilesOnly"){
          configuration.keepCompilesOnly = (value == "true" || value == "True");
        } else if (key == "wipeOldBenchmarks") {
          configuration.wipeOldBenchmarks = (value == "true" || value == "True");
        }
      }
    }
    file.close();
  } else {
    std::cerr << "File Failed to Open" << std::endl;
    std::cout << "Using Default Settings" << std::endl;
  }
}

// Main function should be transfered to a driver for use via the full implementation
int Transformer::run() {
  std::filesystem::path path(configuration.filterDir);
  if (std::filesystem::exists(path)) {
    // run the transformer on the file structure
    int result = transformAll(path, 0);
    std::cout << "Number of Compilable Benchmarks: " << result << std::endl;
    return result;
  }
  return 0;
}
