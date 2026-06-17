#include "include/Filterer.hpp"
#include "FrontendFactoryWithArgs.hpp"
#include "ClangToolUtils.hpp"

#include <algorithm>
#include <clang/AST/Type.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <regex>
#include <sstream>
#include <string>

const std::string defaultFilterDir = "filteredFiles";
const std::string defaultDatabaseDir = "database";
/// Not yet implemented in code - currently handled by scripts
const bool defaultWipeOldBenchmarks = true;

Filterer::Filterer(std::string configFile) {
  configuration.filterDir = defaultFilterDir;
  configuration.databaseDir = defaultDatabaseDir;
  configuration.wipeOldBenchmarks = defaultWipeOldBenchmarks;
  parseConfigFile(configFile);
};

void Filterer::parseConfigFile(std::string configFile) {
  if (!std::filesystem::exists(configFile)) {
    std::cerr << "Config file not found: " << configFile << " — using defaults" << std::endl;
    return;
  }
  for (auto [key, value] : parseIniFile(configFile)) {
    if (config.count(key)) {
      try {
        int i = std::stoi(value);
        config.at(key) = i;
      } catch (...) {
      }
      if (value == "false" || value == "False") {
        config.at(key) = 0;
      } else if (value == "true" || value == "True") {
        config.at(key) = 1;
      }
    } else if (key == "type" || key == "Type") {
      std::regex typePattern("([\\w]+)");
      std::smatch typeMatches;
      while (std::regex_search(value, typeMatches, typePattern)) {
        if (typeMatches[0] == "int" || typeMatches[0] == "Int") {
          typesRequested.push_back(clang::BuiltinType::Int);
          typeNames.push_back(typeMatches[0]);
        } else if (typeMatches[0] == "float" || typeMatches[0] == "Float") {
          typesRequested.push_back(clang::BuiltinType::Float);
          typeNames.push_back(typeMatches[0]);
        } else if (typeMatches[0] == "long" || typeMatches[0] == "Long") {
          typesRequested.push_back(clang::BuiltinType::Long);
          typeNames.push_back(typeMatches[0]);
        } else if (typeMatches[0] == "bool" || typeMatches[0] == "Bool") {
          typesRequested.push_back(clang::BuiltinType::Bool);
          typeNames.push_back(typeMatches[0]);
        } else if (typeMatches[0] == "char" || typeMatches[0] == "Char") {
          typesRequested.push_back(clang::BuiltinType::UChar);
          typeNames.push_back(typeMatches[0]);
        } else {
          std::cerr << "Warning: unrecognised type '" << typeMatches[0] << "' ignored" << std::endl;
        }
        value = typeMatches.suffix().str();
      }
    } else if (key == "databaseDir") {
      configuration.databaseDir = value;
      if (!std::filesystem::exists(value))
        std::cerr << "Database directory not found: " << value << std::endl;
    } else if (key == "filterDir") {
      configuration.filterDir = value;
      if (!std::filesystem::exists(value))
        std::filesystem::create_directory(value);
    } else if (key == "wipeOldBenchmarks") {
      configuration.wipeOldBenchmarks = (value == "true" || value == "True");
    } else if (key == "benchmarkDir" || key == "keepCompilesOnly" || key == "csv" ||
               key == "downloadDir" || key == "language" || key == "projectCount" ||
               key == "minNumStars" || key == "minRepoLoc") {
      // consumed by the transform step or Downloader.py; not a filter key
    } else if (key == "debug") {
      // silently ignored: replaced by debugLevel
    } else {
      std::cerr << "Unknown config key: " << key << std::endl;
    }
  }

  if (config.at("debugLevel") >= 1) {
    std::cout << "[filter] loaded config: " << configFile << std::endl;
    for (const auto &[k, v] : config) {
      std::cout << "  " << k << " = " << v << std::endl;
    }
    for (const std::string &t : typeNames) {
      std::cout << "  type: " << t << std::endl;
    }
  }
}

bool Filterer::checkPotentialFile(std::string fileName) {
  std::ifstream file(fileName);
  std::stringstream buffer;

  if (file.is_open()) {
    std::regex allowedHeadersPattern("#(include|import)\\ *[<\"]([\\w\\/0-9\\.]*)[\">]");
    std::string line;
    std::smatch match;
    int count = 0;
    while (std::getline(file, line)) {
      if (std::regex_search(line, match, allowedHeadersPattern)) {
        bool isStdLib =
            std::find(stdLibNames.begin(), stdLibNames.end(), match[2]) != stdLibNames.end();
        if (!isStdLib && !config.at("useNonStdHeaders")) {
          file.close();
          return false;
        }
      }
      if (line != "") {
        count++;
      }
      buffer << line << std::endl;
    }
    file.close();
    if (count < config.at("minFileLoC")) {
      return false;
    } else if (count > config.at("maxFileLoC")) {
      return false;
    } else {
      return true;
    }
  } else {
    std::cerr << "Failed to open file: " << fileName << std::endl;
    return false;
  }
}

int Filterer::getAllCFiles(std::filesystem::path pathObject,
                           std::vector<std::string> &filesToFilter, int numFiles) {
  if (!std::filesystem::exists(pathObject)) {
    if (config.at("debugLevel") >= 2) {
      std::cout << "[filter] path does not exist: " << pathObject.string() << std::endl;
    }
    return 0;
  }
  if (std::filesystem::is_regular_file(pathObject)) {
    if (pathObject.has_extension()) {
      if (pathObject.extension() == ".c") {
        if (config.at("debugLevel") >= 2) {
          std::cout << "[filter] queued: " << pathObject.filename().string() << std::endl;
        }
        filesToFilter.push_back(pathObject.string());
        return 1;
      } else {
        if (config.at("debugLevel") >= 3) {
          std::cout << "[filter] skipped (not .c): " << pathObject.filename().string() << std::endl;
        }
        return 0;
      }
    } else {
      if (config.at("debugLevel") >= 3) {
        std::cout << "[filter] skipped (no extension): " << pathObject.filename().string()
                  << std::endl;
      }
      return 0;
    }
  } else if (std::filesystem::is_directory(pathObject)) {
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(pathObject)) {
      numFiles += getAllCFiles(entry.path(), filesToFilter);
    }
    return numFiles;
  } else {
    if (config.at("debugLevel") >= 3) {
      std::cout << "[filter] ignored: " << pathObject.filename().string() << std::endl;
    }
    return 0;
  }
  return 0;
}

void Filterer::debugInfo(std::string info) {
  if (config.at("debugLevel") > 0) {
    std::cout << "[filter] " << info << std::endl;
  }
}

/// Main driver for the Filter System
int Filterer::run() {
  std::filesystem::path pathObject(configuration.databaseDir);
  std::vector<std::string> filesToFilter = std::vector<std::string>();

  if (config.at("debugLevel") >= 1) {
    std::cout << "[filter] scanning: " << pathObject.string() << std::endl;
  }

  int filesFound = getAllCFiles(pathObject, filesToFilter, 0);
  if (config.at("debugLevel") >= 1) {
    std::cout << "[filter] found " << filesFound << " .c file(s)" << std::endl;
  }

  for (std::string fileName : filesToFilter) {
    if (checkPotentialFile(fileName)) {
      std::filesystem::path oldPath(fileName);
      std::filesystem::path newPath(std::filesystem::current_path() / configuration.filterDir);
      for (const std::filesystem::path &component : oldPath) {
        if (component.string() != ".." && component.string() != configuration.databaseDir) {
          newPath /= component;
        }
      }

      static llvm::cl::OptionCategory myToolCategory("filterer");

      clang::IgnoringDiagConsumer diagConsumer;

      std::optional<std::string> resourceDir = getResourceDir();
      if (!resourceDir) {
        std::cerr << "Could not determine clang resource directory (set CLANG_RESOURCES to override)"
                  << std::endl;
        return 1;
      }

      std::vector<std::string> args = buildClangArgs(oldPath.string(), *resourceDir);
      std::vector<const char *> argv = toArgv(args);
      int argc = static_cast<int>(args.size());

      std::filesystem::create_directories(newPath.parent_path());

      std::error_code ec;
      llvm::raw_fd_ostream output(llvm::StringRef(newPath.string()), ec);

      llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser =
          clang::tooling::CommonOptionsParser::create(argc, argv.data(), myToolCategory);
      if (!expectedParser) {
        llvm::errs() << expectedParser.takeError();
        return 1;
      }

      clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();
      clang::tooling::ClangTool tool(optionsParser.getCompilations(),
                                     optionsParser.getSourcePathList());
      tool.setDiagnosticConsumer(&diagConsumer);

      FrontendFactoryWithArgs factory(&config, typesRequested, output);

      int result = tool.run(&factory);
      if (config.at("debugLevel") >= 2) {
        std::cout << "[filter] tool result for " << fileName << ": " << result << std::endl;
      }

      output.close();

      if (config.at("debugLevel") >= 2 && std::filesystem::exists(newPath)) {
        std::ifstream outFile(newPath.string());
        if (outFile.is_open()) {
          std::cout << "[filter] output for " << newPath.string() << ":\n"
                    << outFile.rdbuf() << std::endl;
        }
      }
    } else {
      if (config.at("debugLevel") >= 1) {
        std::cout << "[filter] skipped (pre-filter): " << fileName << std::endl;
      }
    }
  }
  return 0;
}
