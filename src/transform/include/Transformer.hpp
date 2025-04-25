#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Transformer {
public:
  bool getFileContents(std::string fileName, std::shared_ptr<std::string> contents);
  bool transformFile(std::filesystem::path path, std::vector<std::string> &args);
  bool transformAll(std::filesystem::path path, std::vector<std::string> &args);
  void parseConfig();
  int run(std::string resources, std::string filePath = "filteredFiles");

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
