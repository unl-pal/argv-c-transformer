#include "include/Transformer.hpp"
#include "ArgsFrontendActionFactory.hpp"
#include "ClangToolUtils.hpp"

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
#include <filesystem>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringRef.h>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
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
  // Apply defaults; parseConfig overrides any keys present in configFile
  configuration.debugLevel = defaultDebugLevel;
  configuration.keepCompilesOnly = defaultKeepCompilesOnly;
  configuration.filterDir = defaultFilterDir;
  configuration.benchmarkDir = defaultBenchmarkDir;
  configuration.wipeOldBenchmarks = defaultWipeOldBenchmarks;
  parseConfig(configFile);
}

bool Transformer::transformFile(std::filesystem::path path) {
  std::cout << "Transforming: " << path.string() << std::endl;
  if (!std::filesystem::exists(path))
    return false;

  // File handling: mirror path under benchmarkDir, dropping the filterDir
  // component so files can't be written outside the project folder
  std::filesystem::path srcPath = std::filesystem::path(configuration.benchmarkDir);
  for (const std::filesystem::path &component : path) {
    if (component.string() != configuration.filterDir && component.string() != "..") {
      srcPath /= component;
    }
  }

  // Tool setup: build the ClangTool from CLANG_RESOURCES and the source path
  static llvm::cl::OptionCategory myToolCategory("transformer");
  clang::IgnoringDiagConsumer diagConsumer;

  std::optional<std::string> resourceDir = getResourceDir();
  if (!resourceDir) {
    std::cerr << "Please set the CLANG_RESOURCES environment variable before proceeding"
              << std::endl;
    return false;
  }

  std::vector<std::string> args = buildClangArgs(path.string(), *resourceDir);
  std::vector<const char *> argv = toArgv(args);
  int argc = static_cast<int>(args.size());

  llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser =
      clang::tooling::CommonOptionsParser::create(argc, argv.data(), myToolCategory);
  if (!expectedParser) {
    llvm::errs() << expectedParser.takeError();
    return false;
  }
  clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();

  clang::tooling::ClangTool tool(optionsParser.getCompilations(),
                                 optionsParser.getSourcePathList());
  tool.setDiagnosticConsumer(&diagConsumer);

  // Invoking: run TransformAction, writing the rewritten source to srcPath
  std::error_code ec;
  std::filesystem::create_directories(srcPath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(srcPath.string()), ec);

  ArgsFrontendFactory factory(output);
  if (tool.run(&factory)) {
    std::cerr << "Clang tool reported errors while transforming: " << path.string() << std::endl;
  }
  output.close();

  // Result: drop the output if it doesn't compile and we're keeping compiles only
  if (!checkCompilable(srcPath)) {
    if (configuration.keepCompilesOnly) {
      std::filesystem::remove(srcPath);
    }
    return 0;
  }
  return 1;
}

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

  std::optional<std::string> resourceDir = getResourceDir();
  if (!resourceDir) {
    return 0;
  }

  std::vector<std::string> args({
      "clang",
      "-extra-arg=-fsyntax-only",
      "-extra-arg=-xc",
      "-extra-arg=-resource-dir=" + *resourceDir,
      path.string(),
      "verifier.c",
  });
  std::vector<const char *> argv = toArgv(args);
  int argc = static_cast<int>(args.size());

  llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser =
      clang::tooling::CommonOptionsParser::create(argc, argv.data(), myToolCategory);

  if (!expectedParser) {
    llvm::errs() << expectedParser.takeError();
    return 0;
  }

  clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();

  clang::tooling::ClangTool tool(optionsParser.getCompilations(),
                                 optionsParser.getSourcePathList());

  // Diagnostics are counted but not printed, to avoid clutter
  clang::DiagnosticConsumer diagConsumer;
  tool.setDiagnosticConsumer(&diagConsumer);

  // Equivalent to running "clang -xc -fsyntax-only `file-name` verifier.c"
  tool.run(clang::tooling::newFrontendActionFactory<clang::SyntaxOnlyAction>().get());

  // If there are errors do not count the file as compilable
  if (diagConsumer.getNumErrors()) {
    return 0;
  }
  return 1;
}

void Transformer::parseConfig(std::string configFile) {
  if (!std::filesystem::exists(configFile)) {
    std::cerr << "Config file not found: " << configFile << " — using defaults" << std::endl;
    return;
  }
  for (const auto &[key, value] : parseIniFile(configFile)) {
    if (key == "benchmarkDir") {
      configuration.benchmarkDir = value;
      if (!std::filesystem::exists(value))
        std::filesystem::create_directory(value);
    } else if (key == "filterDir") {
      configuration.filterDir = value;
      if (!std::filesystem::exists(value))
        std::cerr << "Filter directory not found: " << value << std::endl;
    } else if (key == "debugLevel") {
      try {
        configuration.debugLevel = std::stoi(value);
      } catch (...) {
        configuration.debugLevel = 0;
      }
    } else if (key == "keepCompilesOnly") {
      configuration.keepCompilesOnly = (value == "true" || value == "True");
    } else if (key == "wipeOldBenchmarks") {
      configuration.wipeOldBenchmarks = (value == "true" || value == "True");
    }
  }
}

int Transformer::run() {
  std::filesystem::path path(configuration.filterDir);
  if (std::filesystem::exists(path)) {
    int result = transformAll(path, 0);
    std::cout << "Number of Compilable Benchmarks: " << result << std::endl;
    return result;
  }
  return 0;
}
