#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Transformer {
public:
  /// Creates the frontend action to transform a file
  bool transformFile(std::filesystem::path path, std::vector<std::string> &args);

  /// Recurses through all files and initializes the transformFile Function
  bool transformAll(std::filesystem::path path, std::vector<std::string> &args);

  /// Parses the configuration file to determine necessary types and counts and
  /// location of files
  void parseConfig();

  /// Driver for the transformer that is called by full or transform to run the
  /// transformer on code specified by the given arguments
  int run(std::string filePath = "filteredFiles");

private:
  struct configs {
    int minLoC;
    int maxLoC;
    int minIf;
    int maxLoop;
    int minLoop;
    int minNumCompareInt;
    int minNumOpBinary;
    int minNumOpUnary;
    int minNumVarInt;
    int maxNumVarFloat;
    int maxNumVarString;
    int maxNumVarPoint;
    int maxNumVarStruct;
  };
};
