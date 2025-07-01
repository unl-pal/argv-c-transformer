#pragma once

#include <filesystem>
#include <string>

struct transformConfigs {
  int debugLevel;
  bool keepCompilesOnly;
  std::string filterDir;
  std::string benchmarkDir;
};

class Transformer {
public:
  /// Constructor
  Transformer(std::string configFile);

  /// Creates the frontend action to transform a file
  bool transformFile(std::filesystem::path path);

  /// Recurses through all files and initializes the transformFile Function
  int transformAll(std::filesystem::path path, int count);

  /// Check that the transformed file compiles without errors
  int checkCompilable(std::filesystem::path path);

  /// Parses the configuration file to determine location of files to filter
  /// where to store benchmarks
  /// what the debug level is
  /// whether or not to keep benchmarks that do not compile
  void parseConfig(std::string configFile);

  /// Driver for the transformer that is called by full or transform to run the
  /// transformer on code specified by the given arguments
  int run();

private:
  // std::filesystem::path _ConfigFile;
  struct transformConfigs configuration;
};
