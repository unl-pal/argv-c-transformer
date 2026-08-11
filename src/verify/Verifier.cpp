// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "include/Verifier.hpp"
#include "include/VerifyAction.hpp"

#include "ClangToolUtils.hpp"
#include "CliArgs.hpp"
#include "DebugLog.hpp"

#include <csignal>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <vector>

// Mirrors Transformer's own fallback default, so a bare Verifier invocation
// lines up with the rest of the "repos" -> "repos-benchmarks" chain.
const std::string defaultTransformDir = "repos-transformed";

Verifier::Verifier(std::string configFile, std::string inputPath) : configuration() {
  config = parsePipelineConfig(configFile);
  configuration.debugLevel = config.fileSettings.at("debugLevel");
  configuration.keepCompilesOnly = config.fileSettings.at("keepCompilesOnly") != 0;
  configuration.fileTimeoutSecs = config.fileSettings.at("fileTimeoutSecs");
  configuration.transformDir =
      config.transformDir.empty() ? defaultTransformDir : config.transformDir;
  if (!inputPath.empty())
    configuration.transformDir = inputPath;
  configuration.benchmarkDir = config.benchmarkDir.empty()
                                   ? inputBaseName(configuration.transformDir) + "-benchmarks"
                                   : config.benchmarkDir;
  globalDebugLevel() = configuration.debugLevel;
}

bool Verifier::verifyFile(std::filesystem::path path) {
  debugLog(1, "[verify] file " + std::to_string(_totalProcessed) + ": " + path.string());
  if (!std::filesystem::exists(path)) {
    debugLog(1, "[verify] path does not exist: " + path.string());
    return false;
  }

  // Transform output is already flat, so the benchmark keeps the filename.
  std::filesystem::path outPath =
      std::filesystem::path(configuration.benchmarkDir) / path.filename();

  std::error_code ec;
  std::filesystem::create_directories(outPath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(outPath.string()), ec);
  if (ec) {
    debugLog(0, "Cannot open output file " + outPath.string() + ": " + ec.message());
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
    debugLog(1, "[verify] clang tool failed on: " + path.string());
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
      debugLog(1, "[verify] discarded (fails compile check): " + outPath.string());
      std::filesystem::remove(outPath);
      return false;
    }
    // Kept for inspection, but not a benchmark: no task file, no .i.
    debugLog(1, "[verify] kept but not a benchmark (fails compile check): " + outPath.string());
    return false;
  }

  writeBenchmarkTask(outPath, *counts);
  if (!preprocess(outPath)) {
    debugLog(0, "Preprocessing failed, discarding: " + outPath.string());
    std::filesystem::path ymlPath = outPath;
    ymlPath.replace_extension(".yml");
    std::filesystem::remove(outPath);
    std::filesystem::remove(ymlPath);
    return false;
  }
  return true;
}

int Verifier::verifyFileIsolated(std::filesystem::path path) {
  pid_t pid = fork();
  if (pid < 0) {
    debugLog(0, "fork failed, verifying in-process: " + path.string());
    return verifyFile(path) ? 1 : 0;
  }

  if (pid == 0) {
    int produced = verifyFile(path) ? 1 : 0;
    std::cout.flush();
    std::cerr.flush();
    _exit(produced);
  }

  time_t deadline = time(nullptr) + configuration.fileTimeoutSecs;
  int status = 0;
  while (true) {
    pid_t done = waitpid(pid, &status, WNOHANG);
    if (done == pid)
      break;
    if (done < 0) {
      debugLog(0, "waitpid failed for " + path.string());
      return 0;
    }
    if (time(nullptr) >= deadline) {
      debugLog(0, "Timeout, killing verify of: " + path.string());
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
  debugLog(0, "Verify crashed (signal " + std::to_string(WTERMSIG(status)) +
                  "), skipping: " + path.string());
  cleanupPartialOutput(path);
  return 0;
}

void Verifier::cleanupPartialOutput(std::filesystem::path path) {
  std::filesystem::path outPath =
      std::filesystem::path(configuration.benchmarkDir) / path.filename();
  std::error_code ec;
  std::filesystem::remove(outPath, ec);
  std::filesystem::path ymlPath = outPath;
  ymlPath.replace_extension(".yml");
  std::filesystem::remove(ymlPath, ec);
  std::filesystem::path iPath = outPath;
  iPath.replace_extension(".i");
  std::filesystem::remove(iPath, ec);
}

int Verifier::verifyAll(std::filesystem::path path) {
  if (!std::filesystem::exists(path)) {
    debugLog(1, "[verify] path does not exist: " + path.string());
    return 0;
  }
  if (std::filesystem::is_directory(path)) {
    int successes = 0;
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(path)) {
      successes += verifyAll(entry.path());
    }
    return successes;
  }
  if (std::filesystem::is_regular_file(path)) {
    if (path.extension() == ".c") {
      _totalProcessed++;
      if (globalDebugLevel() == 0)
        std::cout << "\r[verify] " << _totalProcessed << " processed" << std::flush;
      return verifyFileIsolated(path);
    }
    debugLog(3, "[verify] skipped (not .c): " + path.filename().string());
    return 0;
  }
  debugLog(3, "[verify] ignored: " + path.filename().string());
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
  std::vector<BenchmarkProperty> properties;
  if (counts.count("reach_error"))
    properties.push_back({"../properties/unreach-call.prp", true});
  bool loopsPresent = false;
  bool intArithPresent = false;
  bool memsafetyPresent = false;
  for (const auto &[name, attr] : counts) {
    if (!loopsPresent && (attr.Complexity.ForLoops || attr.Complexity.WhileLoops)) {
      loopsPresent = true;
      properties.push_back({"../properties/termination.prp", true});
    }
    if (!intArithPresent && attr.Complexity.Operations) {
      // The operations check is pretty naive as it counts any/all binary and unary operations with side effects
      intArithPresent = true;
      properties.push_back({"../properties/no-overflow.prp", true});
    }
    // valid-memsafety.prp bundles deref/free/memtrack CHECKs in one file
    if (!memsafetyPresent &&
        (attr.Features.PointerDeref || attr.Features.MemAlloc || attr.Features.MemFree)) {
      memsafetyPresent = true;
      properties.push_back({"../properties/valid-memsafety.prp", true});
    }
    if (loopsPresent && intArithPresent && memsafetyPresent)
      break;
  }
  return properties;
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
    debugLog(0, "Failed to write task file: " + ymlPath.string());
    return;
  }

  out << "format_version: '2.0'\n"
      << "\n"
      << "input_files: '" << inputFile << "'\n"
      << "\n";
  if (properties.empty()) {
    out << "properties: []\n";
  } else {
    out << "properties:\n";
    for (const BenchmarkProperty &prop : properties) {
      out << "  - property_file: " << prop.propertyFile << "\n"
          << "    expected_verdict: " << (prop.expectedVerdict ? "true" : "false") << "\n";
    }
  }
  out << "\n"
      << "options:\n"
      << "  language: C\n"
      << "  data_model: LP64\n";
  if (properties.empty())
    debugLog(1, "[verify] WARN: no properties found for " + cPath.string());
}

namespace {

// Lines matching one of these are pure header noise. Each entry is a case a specific verifier
// frontend chokes on despite the declaration being semantically inert.
//
// - _Float16/32/64/128(x): glibc's <bits/floatn-common.h> fallback typedefs
//   for the C23 extended float types, pulled in transitively by <stdio.h>
const std::vector<std::regex> &knownNoiseTypedefs() {
  static const std::vector<std::regex> patterns = {
      std::regex(R"(^\s*typedef\s+.+\s_Float\d+x?\s*;\s*$)"),
  };
  return patterns;
}

// Strips knownNoiseTypedefs() lines from an already-preprocessed .i file.
void stripNoiseTypedefs(const std::filesystem::path &iPath) {
  std::ifstream in(iPath);
  if (!in)
    return;
  std::vector<std::string> kept;
  std::string line;
  bool changed = false;
  while (std::getline(in, line)) {
    bool isNoise = false;
    for (const std::regex &pattern : knownNoiseTypedefs()) {
      if (std::regex_match(line, pattern)) {
        isNoise = true;
        break;
      }
    }
    if (isNoise) {
      changed = true;
      continue;
    }
    kept.push_back(std::move(line));
  }
  in.close();
  if (!changed)
    return;
  std::ofstream out(iPath, std::ios::trunc);
  for (const std::string &l : kept)
    out << l << "\n";
}

} // namespace

// NOTE: same std::system/unescaped-path caveat as checkCompilable above.
bool Verifier::preprocess(std::filesystem::path cPath) {
  std::filesystem::path iPath = cPath;
  iPath.replace_extension(".i");

  std::optional<std::string> cmd = clangCommand("-E -P -std=gnu11");
  if (!cmd)
    return false;
  *cmd += " " + cPath.string() + " -o " + iPath.string() + " 2>/dev/null";
  if (std::system(cmd->c_str()) != 0)
    return false;
  stripNoiseTypedefs(iPath);
  return true;
}

int Verifier::run() {
  std::filesystem::path path(configuration.transformDir);
  if (!std::filesystem::exists(path)) {
    debugLog(0, "Transform directory not found: " + configuration.transformDir);
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
