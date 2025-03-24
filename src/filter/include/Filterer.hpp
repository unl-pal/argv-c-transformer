#pragma once

#include "Filter.h"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Filterer {
public:
  Filterer();

  void parseConfigFile(std::string configFile);

  bool checkPotentialFile(std::string fileName, std::shared_ptr<std::string> contents);

  int getAllCFiles(std::filesystem::path pathObject,
           std::vector<std::string> &filesToFilter,
           int numFiles = 0);

  std::vector<std::string> getPathDirectories();

  std::vector<std::string> filterFunctions(std::unordered_map<std::string, CountNodesVisitor::attributes*> functions);

  void debugInfo(std::string info);

  int run(int argc, char** argv);

private:
  /// vector of all standard library names to compare includes to
  const std::vector<std::string> stdLibNames =
  { "assert.h",
    "complex.h",
    "ctype.h",
    "errno.h",
    "fenv.h",
    "float.h",
    "inttypes.h",
    "iso646.h",
    "limits.h",
    "locale.h",
    "math.h",
    "setjmp.h",
    "signal.h",
    "stdalign.h",
    "stdarg.h",
    "stdatomic.h",
    "stdbit.h",
    "stdbool.h",
    "stdckdint.h",
    "stddef.h",
    "stdint.h",
    "stdio.h",
    "stdlib.h",
    "stdmchar.h",
    "stdnoreturn.h",
    "string.h",
    "tgmath.h",
    "threads.h",
    "time.h",
    "uchar.h",
    "wchar.h",
    "wctype.h"
  };
  /// Map of Valid Config Settings with Default Values
  std::map<std::string, int> config = {
    {"maxNumLoopFor", 2},
    {"maxNumLoopWhile", 2},
    {"minNumIfStmt", 1},
    {"minNumVarRefInt", 1},
    {"minNumOpCompare", 1},
    {"minFileLoC", 5},
    {"maxFileLoC", 2000},
    {"debug", 1},
    {"useNonStdHeaders", 0},
  };
};
