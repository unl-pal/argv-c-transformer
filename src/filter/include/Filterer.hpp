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
    {"minFileLoC", 5},
    {"maxFileLoC", 2000},
    {"debug", 1},
    {"debugLevel", 0},
    {"useNonStdHeaders", 0},
    {"maxNumCallFunc", 9999},
    {"minNumCallFunc", 0},
		{"maxNumCompChar", 9999},
		{"minNumCompChar", 0},
		{"maxNumCompFloat", 9999},
		{"minNumCompFloat", 0},
		{"maxNumCompInt", 9999},
		{"minNumCompInt", 0},
		{"maxNumFunctions", 9999},
		{"minNumFunctions", 0},
		{"maxNumIfStmt", 9999},
    {"minNumIfStmt", 1},
		{"maxNumIfStmtInt", 9999},
		{"minNumIfStmtInt", 0},
    {"maxNumLoopFor", 2},
		{"minNumLoopFor", 0},
    {"maxNumLoopWhile", 2},
		{"minNumLoopWhile", 0},
		{"maxNumOpBinary", 9999},
		{"minNumOpBinary", 0},
		{"maxNumOpCompare", 9999},
    {"minNumOpCompare", 1},
		{"maxNumOpCondition", 9999},
		{"minNumOpCondition", 0},
		{"maxNumOpUnary", 9999},
		{"minNumOpUnary", 0},
		{"maxNumVarFloat", 9999},
		{"minNumVarFloat", 0},
		{"maxNumVarInt", 9999},
		{"minNumVarInt", 0},
		{"maxNumVarPoint", 9999},
		{"minNumVarPoint", 0},
		{"maxNumVarRefArray", 9999},
		{"minNumVarRefArray", 0},
		{"maxNumVarRefCompare", 9999},
		{"minNumVarRefCompare", 0},
		{"maxNumVarRefInt", 9999},
    {"minNumVarRefInt", 1},
		{"maxNumVarRefStruct", 9999},
		{"minNumVarRefStruct", 0},
		{"maxNumVarStruct", 9999},
		{"minNumVarStruct", 0},
  };
};
