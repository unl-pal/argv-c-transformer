// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Verifier.hpp"
#include "include/VerifyAction.hpp"

#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include "DebugLog.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <optional>
#include <string>
#include <system_error>

const std::string defaultTransformDir = "transformedFiles";
const std::string defaultBenchmarkDir = "benchmarks";

Verifier::Verifier(std::string configFile, std::string inputPath) : configuration() {
  // Apply defaults, then let the config file override them. A command-line
  // input path always wins over config's transformDir and benchmarkDir
  // (the transformDir-named guard below just avoids deriving
  // "transformedFiles-benchmarks" when the input is literally that default
  // directory name).
  config = parsePipelineConfig(configFile);
  configuration.debugLevel = config.fileSettings.at("debugLevel");
  configuration.keepCompilesOnly = config.fileSettings.at("keepCompilesOnly") != 0;
  configuration.transformDir =
      config.transformDir.empty() ? defaultTransformDir : config.transformDir;
  configuration.benchmarkDir =
      config.benchmarkDir.empty() ? defaultBenchmarkDir : config.benchmarkDir;
  if (!inputPath.empty()) {
    configuration.transformDir = inputPath;
    if (inputBaseName(inputPath) != defaultTransformDir)
      configuration.benchmarkDir = inputBaseName(inputPath) + "-benchmarks";
  }
  globalDebugLevel() = configuration.debugLevel;
}

bool Verifier::verifyFile(std::filesystem::path path) {
  debugLog(1, "Verifying: " + path.string());
  if (!std::filesystem::exists(path))
    return false;

  // Transform output is already flat, so the benchmark keeps the filename.
  std::filesystem::path outPath =
      std::filesystem::path(configuration.benchmarkDir) / path.filename();

  std::error_code ec;
  std::filesystem::create_directories(outPath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(outPath.string()), ec);
  if (ec) {
    std::cerr << "Cannot open output file " << outPath.string() << ": " << ec.message()
              << std::endl;
    return false;
  }

  // Shared state the driver reads back after the tool run: fresh counts (for
  // property selection) and the rejected names.
  auto counts = std::make_shared<std::unordered_map<std::string, CountingVisitor::attributes>>();
  auto toRemove = std::make_shared<std::vector<std::string>>();

  VerifyActionFactory factory(&config.complexity, &config.features, counts, toRemove, output);
  bool ran = runToolOnFile(path.string(), factory);
  output.close();
  if (!ran) {
    std::filesystem::remove(outPath);
    return false;
  }

  if (harnessIsEmpty(outPath)) {
    debugLog(1, "[verify] discarded (harness empty after re-check): " + outPath.string());
    std::filesystem::remove(outPath);
    return false;
  }

  // Drop the output if it doesn't compile and we're keeping compiles only
  if (!checkCompilable(outPath)) {
    if (configuration.keepCompilesOnly) {
      std::filesystem::remove(outPath);
      return false;
    }
    // Kept for inspection, but not a benchmark: no task file, no .i.
    return false;
  }

  writeBenchmarkTask(outPath, *counts);
  if (!preprocess(outPath)) {
    std::cerr << "Preprocessing failed, discarding: " << outPath.string() << std::endl;
    std::filesystem::path ymlPath = outPath;
    ymlPath.replace_extension(".yml");
    std::filesystem::remove(outPath);
    std::filesystem::remove(ymlPath);
    return false;
  }
  return true;
}

int Verifier::verifyAll(std::filesystem::path path) {
  if (!std::filesystem::exists(path))
    return 0;
  if (std::filesystem::is_directory(path)) {
    int successes = 0;
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(path)) {
      successes += verifyAll(entry.path());
    }
    return successes;
  }
  if (std::filesystem::is_regular_file(path) && path.extension() == ".c") {
    _totalProcessed++;
    return verifyFile(path) ? 1 : 0;
  }
  return 0;
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

// NOTE: cmd is passed to std::system (shell-interpreted), and path/verifierPath
// are not escaped. path originates from a cloned/downloaded repository, so a
// pathological filename containing shell metacharacters could inject commands
bool Verifier::checkCompilable(std::filesystem::path path) {
  std::optional<std::string> cmd = clangCommand("-fsyntax-only -xc");
  if (!cmd)
    return false;

  std::filesystem::path verifierPath = path.parent_path() / "__verifier_stubs.c";
  {
    std::ofstream out(verifierPath);
    out << kVerifierStubs;
  }

  *cmd += " " + path.string() + " " + verifierPath.string() + " 2>/dev/null";
  int result = std::system(cmd->c_str());
  std::filesystem::remove(verifierPath);
  return result == 0;
}

std::vector<BenchmarkProperty> Verifier::selectProperties(
    const std::unordered_map<std::string, CountingVisitor::attributes> &counts) {
  // TODO: use the per-function counts/features to conditionally include
  // properties (loops → termination, int arithmetic → no-overflow, etc.).
  // For now, every benchmark gets both.
  (void)counts;
  return {
      {"../properties/no-overflow.prp", true},
      {"../properties/termination.prp", true},
  };
}

void Verifier::writeBenchmarkTask(
    std::filesystem::path cPath,
    const std::unordered_map<std::string, CountingVisitor::attributes> &counts) {
  std::filesystem::path ymlPath = cPath;
  ymlPath.replace_extension(".yml");

  std::string inputFile = cPath.stem().string() + ".i";
  std::vector<BenchmarkProperty> properties = selectProperties(counts);

  std::ofstream out(ymlPath);
  if (!out) {
    std::cerr << "Failed to write task file: " << ymlPath.string() << std::endl;
    return;
  }

  out << "format_version: '2.0'\n"
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

// NOTE: same std::system/unescaped-path caveat as checkCompilable above.
bool Verifier::preprocess(std::filesystem::path cPath) {
  std::filesystem::path iPath = cPath;
  iPath.replace_extension(".i");

  std::optional<std::string> cmd = clangCommand("-E -P -std=gnu11");
  if (!cmd)
    return false;
  *cmd += " " + cPath.string() + " -o " + iPath.string() + " 2>/dev/null";
  return std::system(cmd->c_str()) == 0;
}

int Verifier::run() {
  std::filesystem::path path(configuration.transformDir);
  if (!std::filesystem::exists(path)) {
    std::cerr << "Transform directory not found: " << configuration.transformDir << std::endl;
    return 0;
  }
  int result = verifyAll(path);
  int discarded = _totalProcessed - result;
  std::cout << "\n=== Verify summary ===\n"
            << "  Files processed:        " << _totalProcessed << "\n"
            << "  Benchmarks produced:    " << result << "\n"
            << "  Discarded/failed:       " << discarded << std::endl;
  return result;
}
