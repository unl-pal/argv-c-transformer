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
#include <clang/Lex/Preprocessor.h>
#include <clang/Serialization/PCHContainerOperations.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <filesystem>
#include <fstream>
#include <sstream>
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
#include <csignal>
#include <ctime>
#include <sys/wait.h>
#include <unistd.h>

const int defaultDebugLevel = 0;
const bool defaultKeepCompilesOnly = true;
const std::string defaultFilterDir = "filteredFiles";
const std::string defaultBenchmarkDir = "benchmarks";
/// Not yet implemented in code - currently handled by scripts
const bool defaultWipeOldBenchmarks = true;
/// Per-file wall-clock budget for the isolated transform child, in seconds.
const int defaultFileTimeoutSecs = 60;

Transformer::Transformer(std::string configFile) : configuration() {
  // Apply defaults; parseConfig overrides any keys present in configFile
  configuration.debugLevel = defaultDebugLevel;
  configuration.keepCompilesOnly = defaultKeepCompilesOnly;
  configuration.filterDir = defaultFilterDir;
  configuration.benchmarkDir = defaultBenchmarkDir;
  configuration.wipeOldBenchmarks = defaultWipeOldBenchmarks;
  configuration.fileTimeoutSecs = defaultFileTimeoutSecs;
  parseConfig(configFile);
}

// Flatten the filtered path into a single filename under benchmarkDir:
//   filtered-files/antirez/redis/src/endianconv.c
//   → transformed-files/antirez_redis_src_endianconv.c
// Strip the filterDir prefix (works for both relative and absolute paths),
// then join remaining components with underscores.
std::filesystem::path Transformer::flattenedOutputPath(std::filesystem::path path) {
  std::filesystem::path relPath = std::filesystem::relative(path, configuration.filterDir);
  std::string flatName;
  for (const std::filesystem::path &component : relPath) {
    std::string part = component.string();
    if (part == ".." || part == ".")
      continue;
    if (!flatName.empty())
      flatName += "_";
    flatName += part;
  }
  return std::filesystem::path(configuration.benchmarkDir) / flatName;
}

bool Transformer::transformFile(std::filesystem::path path) {
  std::cout << "Transforming: " << path.string() << std::endl;
  if (!std::filesystem::exists(path))
    return false;

  std::filesystem::path srcPath = flattenedOutputPath(path);

  // Tool setup: build the ClangTool from CLANG_RESOURCES and the source path
  static llvm::cl::OptionCategory myToolCategory("transformer");
  clang::IgnoringDiagConsumer diagConsumer;

  std::optional<std::string> resourceDir = getResourceDir();
  if (!resourceDir) {
    std::cerr << "Could not determine clang resource directory (set CLANG_RESOURCES to override)"
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

  // harness may be empty due to unsupported transforming
  if (harnessIsEmpty(srcPath)) {
    std::filesystem::remove(srcPath);
    return 0;
  }

  // Result: drop the output if it doesn't compile and we're keeping compiles only
  if (!checkCompilable(srcPath)) {
    if (configuration.keepCompilesOnly) {
      std::filesystem::remove(srcPath);
    }
    return 0;
  }
  writeBenchmarkTask(srcPath);
  if (!preprocess(srcPath)) {
    std::cerr << "Preprocessing failed, discarding: " << srcPath.string() << std::endl;
    std::filesystem::path ymlPath = srcPath;
    ymlPath.replace_extension(".yml");
    std::filesystem::remove(srcPath);
    std::filesystem::remove(ymlPath);
    return 0;
  }
  return 1;
}

// Runs transformFile in a forked child so a crash, OOM-kill, assertion, or
// hang on a single pathological file cannot take down the whole batch. The
// child does all the file I/O (rewritten .c, .yml, .i) and exits with 1 on a
// produced benchmark, 0 otherwise; the parent enforces a wall-clock timeout
// and reaps the child, translating its fate into the success count.
int Transformer::transformFileIsolated(std::filesystem::path path) {
  pid_t pid = fork();
  if (pid < 0) {
    std::cerr << "fork failed, transforming in-process: " << path.string() << std::endl;
    return transformFile(path) ? 1 : 0;
  }

  if (pid == 0) {
    // Child: do the work and report 1/0 through the exit status. _exit skips
    // C++ stream flushing, so flush explicitly first (stdout may be fully
    // buffered when redirected, e.g. under benchexec).
    int produced = transformFile(path) ? 1 : 0;
    std::cout.flush();
    std::cerr.flush();
    _exit(produced);
  }

  // Parent: poll for completion, killing the child if it overruns the budget.
  time_t deadline = time(nullptr) + configuration.fileTimeoutSecs;
  int status = 0;
  while (true) {
    pid_t done = waitpid(pid, &status, WNOHANG);
    if (done == pid)
      break;
    if (done < 0) {
      std::cerr << "waitpid failed for " << path.string() << std::endl;
      return 0;
    }
    if (time(nullptr) >= deadline) {
      std::cerr << "Timeout, killing transform of: " << path.string() << std::endl;
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      cleanupPartialOutput(path);
      return 0;
    }
    struct timespec nap = {0, 20 * 1000 * 1000}; // 20ms
    nanosleep(&nap, nullptr);
  }

  if (WIFEXITED(status))
    return WEXITSTATUS(status) == 1 ? 1 : 0;
  // WIFSIGNALED: segfault, OOM-kill, etc. The child may have left a partial
  // .c/.yml behind; harnessIsEmpty/keepCompilesOnly never ran, so clean up.
  std::cerr << "Transform crashed (signal " << WTERMSIG(status) << "), skipping: "
            << path.string() << std::endl;
  cleanupPartialOutput(path);
  return 0;
}

// Remove any .c/.yml/.i a crashed or timed-out child left half-written, so
// downstream steps never see a partial benchmark.
void Transformer::cleanupPartialOutput(std::filesystem::path path) {
  std::filesystem::path srcPath = flattenedOutputPath(path);
  std::error_code ec;
  for (const char *ext : {".c", ".yml", ".i"}) {
    std::filesystem::path p = srcPath;
    p.replace_extension(ext);
    std::filesystem::remove(p, ec);
  }
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
        return count + transformFileIsolated(path);
      }
    }
  }
  return count;
}

static constexpr const char *kVerifierStubs = R"(
#include <stdbool.h>
#include <stddef.h>
bool __VERIFIER_nondet_bool(void) { return false; }
char __VERIFIER_nondet_char(void) { return 'a'; }
unsigned char __VERIFIER_nondet_uchar(void) { return 'a'; }
short __VERIFIER_nondet_short(void) { return 0; }
unsigned short __VERIFIER_nondet_ushort(void) { return 0; }
int __VERIFIER_nondet_int(void) { return 0; }
unsigned int __VERIFIER_nondet_uint(void) { return 0; }
long __VERIFIER_nondet_long(void) { return 0; }
unsigned long __VERIFIER_nondet_ulong(void) { return 0; }
long long __VERIFIER_nondet_longlong(void) { return 0; }
unsigned long long __VERIFIER_nondet_ulonglong(void) { return 0; }
float __VERIFIER_nondet_float(void) { return 0; }
double __VERIFIER_nondet_double(void) { return 0; }
void* __VERIFIER_nondet_pointer(void) { return (void*)(0); }
void __VERIFIER_nondet_memory(void *mem, size_t size) {
  unsigned char *p = (unsigned char *)mem;
  for (size_t i = 0; i < size; i++) p[i] = __VERIFIER_nondet_uchar();
}
void reach_error(void) {}
)";

int Transformer::checkCompilable(std::filesystem::path path) {
  std::optional<std::string> resourceDir = getResourceDir();
  if (!resourceDir)
    return 0;

  std::filesystem::path verifierPath = path.parent_path() / "__verifier_stubs.c";
  {
    std::ofstream out(verifierPath);
    out << kVerifierStubs;
  }

  std::string cmd = "clang -fsyntax-only -xc"
                    " -resource-dir=" + *resourceDir;
  std::optional<std::string> sysroot = getSysroot();
  if (sysroot)
    cmd += " -isysroot " + *sysroot;
  cmd += " " + path.string() + " " + verifierPath.string() + " 2>/dev/null";

  int result = std::system(cmd.c_str());
  std::filesystem::remove(verifierPath);
  return result == 0 ? 1 : 0;
}

bool Transformer::harnessIsEmpty(std::filesystem::path path) {
  std::ifstream in(path);
  if (!in)
    return false;
  std::stringstream buffer;
  buffer << in.rdbuf();
  std::string content = buffer.str();

  // MainGenConsumer builds the entry point as
  //   "\nint main(void) {\n" + harness + "  return 0;\n}\n"
  // so when no function could be harnessed (harness is empty), this exact
  // block appears verbatim. Coupled to that format string in MainGenConsumer.
  return content.find("int main(void) {\n  return 0;\n}") != std::string::npos;
}

std::vector<BenchmarkProperty> Transformer::selectProperties() {
  // TODO(you): later, accept AST characteristics and conditionally include
  // properties (loops → termination, int arithmetic → no-overflow, etc.).
  // For now, every benchmark gets both.
  return {
      {"../properties/no-overflow.prp", true},
      {"../properties/termination.prp", true},
  };
}

void Transformer::writeBenchmarkTask(std::filesystem::path cPath) {
  std::filesystem::path ymlPath = cPath;
  ymlPath.replace_extension(".yml");

  std::string inputFile = cPath.stem().string() + ".i";
  std::vector<BenchmarkProperty> properties = selectProperties();

  std::ofstream out(ymlPath);
  if (!out) {
    std::cerr << "Failed to write task file: " << ymlPath.string() << std::endl;
    return;
  }

  out << "# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project\n"
      << "# SPDX-License-Identifier: Apache-2.0\n"
      << "\n"
      << "format_version: '2.0'\n"
      << "\n"
      << "input_files: '" << inputFile << "'\n"
      << "\n"
      << "properties:\n";
  for (const BenchmarkProperty &prop : properties) {
    out << "  - property_file: " << prop.propertyFile << "\n"
        << "    expected_verdict: " << (prop.expectedVerdict ? "true" : "false") << "\n";
  }
  out << "\n"
      << "options:\n"
      << "  language: C\n"
      << "  data_model: LP64\n";
}

bool Transformer::preprocess(std::filesystem::path cPath) {
  std::filesystem::path iPath = cPath;
  iPath.replace_extension(".i");

  std::string cmd = "gcc -E -P -std=gnu11 " +
                    cPath.string() + " -o " + iPath.string() + " 2>/dev/null";
  return std::system(cmd.c_str()) == 0;
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
    } else if (key == "fileTimeoutSecs") {
      try {
        configuration.fileTimeoutSecs = std::stoi(value);
      } catch (...) {
        configuration.fileTimeoutSecs = defaultFileTimeoutSecs;
      }
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
