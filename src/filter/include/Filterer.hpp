#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct filterConfigs {
  std::string databaseDir;
  std::string filterDir;
  int debugLevel;
};

class Filterer {
public:
  /// Constructor for the Filterer Object
  Filterer(std::string configFile);

  /// Parse the config file creating the config map used by the rest of the
  /// filter steps
  void parseConfigFile(std::string configFile);

  /// Using the filename and contents this function iterates through the file
  /// and adds to the contents pointer as needed returning true on a file that
  /// has potential for our tool or false on a undesirable file. The contents
  /// are only populated in the case of the file being desireable
  bool checkPotentialFile(std::string                  fileName,
                          std::shared_ptr<std::string> contents);

  /// Finds all C files in a path 
  /// single file path or dir are both acceptable
  int getAllCFiles(std::filesystem::path     pathObject,
                   std::vector<std::string> &filesToFilter, int numFiles = 0);

  /// Debugger that is only partially implemented and not ready for use
  void debugInfo(std::string info);

  /// Main driver for the rest of the code creating the filter tool and running
  /// it on each file found in the path that has potential
  int run();

private:
  /// vector of all standard library names to compare includes to
  const std::vector<std::string> stdLibNames = {
    "assert.h",    "complex.h",  "ctype.h",   "errno.h",     "fenv.h",
    "float.h",     "inttypes.h", "iso646.h",  "limits.h",    "locale.h",
    "math.h",      "setjmp.h",   "signal.h",  "stdalign.h",  "stdarg.h",
    "stdatomic.h", "stdbit.h",   "stdbool.h", "stdckdint.h", "stddef.h",
    "stdint.h",    "stdio.h",    "stdlib.h",  "stdmchar.h",  "stdnoreturn.h",
    "string.h",    "tgmath.h",   "threads.h", "time.h",      "uchar.h",
    "wchar.h",     "wctype.h"};

  std::vector<unsigned int> typesRequested;
  std::vector<std::string> typeNames;

  /// Map of Valid Config Settings with Default Values
  std::map<std::string, int> *config = new std::map<std::string, int>({
    {"debug", 1},
    // {"debugLevel", 0},
    {"maxCallFunc", 99999},
    {"maxFileLoC", 2000},
    {"maxForLoops", 99999},
    {"maxFunctions", 99999},
    {"maxIfStmt", 99999},
    {"maxParam", 99999},
    {"maxTypeArithmeticOperation", 99999},
    {"maxTypeCompareOperation", 99999},
    {"maxTypeComparisons", 99999},
    {"maxTypeIfStmt", 99999},
    {"maxTypeParameters", 99999},
    {"maxTypePostfix", 99999},
    {"maxTypePrefix", 99999},
    {"maxTypeUnaryOperation", 99999},
    {"maxTypeVariableReference", 99999},
    {"maxTypeVariables", 99999},
    {"maxWhileLoops", 99999},
    {"minCallFunc", 0},
    {"minFileLoC", 5},
    {"minForLoops", 0},
    {"minFunctions", 0},
    {"minIfStmt", 0},
    {"minParam", 0},
    {"minTypeArithmeticOperation", 0},
    {"minTypeCompareOperation", 0},
    {"minTypeComparisons", 0},
    {"minTypeIfStmt", 0},
    {"minTypeParameters", 0},
    {"minTypePostfix", 0},
    {"minTypePrefix", 0},
    {"minTypeUnaryOperation", 0},
    {"minTypeVariableReference", 0},
    {"minTypeVariables", 0},
    {"minWhileLoops", 0},
    {"useNonStdHeaders", 0}
  });
  struct filterConfigs configuration;
};
