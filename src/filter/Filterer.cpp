#include "include/Filterer.hpp"
#include "FrontendFactoryWithArgs.hpp"

#include <algorithm>
#include <clang/AST/Type.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <regex>
#include <string>

Filterer::Filterer(std::string configFile){
  typesRequested = std::vector<unsigned int>();
  typeNames = std::vector<std::string>();
  parseConfigFile(configFile);

};

// Parse the config file for all settings as well as a list of desired types
/// TODO MAKE THE PARSERS MORE SECURE!!
void Filterer::parseConfigFile(std::string configFile) {
  std::ifstream file(configFile);
  if (!std::filesystem::exists(configFile)) {
    std::cout << "File: " << configFile << " Does Not Exist" << std::endl;
    std::cout << "Using Default Settings" << std::endl;
    return;
  }
  if (file.is_open()) {
    std::cout << "Using: " << configFile << " Specified Settings" << std::endl;
    std::regex pattern("^\\s*(\\w+)\\s*=\\s*([0-9]+|[\\w\\s,]+|[\\w/-_.]+)$");
    std::string line;
    std::smatch match;
    while (std::getline(file, line)) {
      if (std::regex_search(line, match, pattern)) {
        std::string key = match[1];
        std::string value = match[2];
        // Add the value to the config if the key is a member of the map
        if (config->count(key)) {
          try {
            int i = std::stoi(value);
            config->at(key) = i;
          } catch (...) {
          } 
          // For true false values convert to 1s and 0s
          if (value == "false" || value == "False") {
            config->at(key) = 0;
          } else if (value == "true" || value == "True") {
            config->at(key) = 1;
          }
        }
        // For types go through all given types and add only supported types to the list
        else if (key == "type" || key == "Type") {
          std::vector<std::string> allTypes = std::vector<std::string>();
          std::regex typePattern("([\\w]+)");
          std::smatch typeMatches;
          std::regex_search(value, typeMatches, typePattern);
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
            }
            value = typeMatches.suffix().str();
          }
        } else if (key == "databaseDir") {
          if (std::filesystem::exists(value)) {
            configuration.databaseDir = value;
          } else {
            configuration.databaseDir = "database";
          }
        } else if (key == "filterDir") {
          if (std::filesystem::exists(value)) {
            configuration.filterDir = value;
          } else {
            configuration.filterDir = "filteredFiles";
          }
        } else if (key == "debugLevel") {
          try {
            configuration.debugLevel = std::stoi(value);
          } catch (...) {
            configuration.debugLevel = 0;
          }
        } else {
          std::cout << "Key: " << key
            << " Is Not A Valid Key For Filtering Files" << std::endl;
        }
      }
    }
    file.close();
    std::cout << "Using Config Settings:" << std::endl;
    for (std::pair item : *config) {
      std::cout << "Property: " << item.first << "=" << item.second << std::endl;
    }
    std::cout << "For Types: " << std::endl;
    for (std::string T : typeNames) {
      std::cout << T << std::endl;
    }
  } else {
    std::cerr << "File Failed to Open" << std::endl;
    std::cout << "Using Default Settings" << std::endl;
    configuration.debugLevel = 0;
    configuration.filterDir = "filtereredFiles";
    configuration.databaseDir = "database";
  }
}

/// Checks a file for compliance with set config properties
/// @param fileName : name of the file to check
/// @param contents : string pointer containing the contents of the file
/// @return : boolean true if the file passes the filter
bool Filterer::checkPotentialFile(std::string fileName,
                                  std::shared_ptr<std::string> contents) {
  std::ifstream file(fileName);
  std::stringstream buffer;
  std::cout << fileName << std::endl;

  if (file.is_open()) {
    std::regex pattern("#(include|import)\\ *[<\"]([\\w\\/0-9\\.]*)[\">]");
    std::string line;
    std::smatch match;
    int count = 0;
    while (std::getline(file, line)) {
      if (std::regex_search(line, match, pattern)) {
        if (std::find(stdLibNames.begin(), stdLibNames.end(), match[2]) !=
            stdLibNames.end()) {
        } else if (!config->at("useNonStdHeaders")) {
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
    if (count < config->at("minFileLoC")) {
      *contents = "";
      return false;
    } else if (count > config->at("maxFileLoC")) {
      *contents = "";
      return false;
    } else {
      *contents += buffer.str();
      return true;
    }
  } else {
    std::cerr << "File Failed to Open" << std::endl;
    return false;
  }
}

// Searches through the directory and identifies all c files, adding to array
int Filterer::getAllCFiles(std::filesystem::path pathObject,
                           std::vector<std::string> &filesToFilter,
                           int numFiles) {
  if (!std::filesystem::exists(pathObject)) {
    if (config->at("debug")) {
      std::cout << "Path: " << " Does Not Exist" << std::endl;
    }
    return 0;
  }
  if (std::filesystem::is_regular_file(pathObject)) {
    if (pathObject.has_extension()) {
      if (pathObject.extension() == ".c") {
        if (config->at("debug")) {
          std::cout << "File: " << pathObject.filename()
                    << " Added To Filter List" << std::endl;
        }
        filesToFilter.push_back(pathObject.string());
        return 1;
      } else {
        if (config->at("debug")) {
          std::cout << "File: " << pathObject.filename() << " is Not a C File"
                    << std::endl;
        }
        return 0;
      }
    } else {
      if (config->at("debug")) {
        std::cout << "File: " << pathObject.filename() << " Has No Extension"
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
    if (config->at("debug")) {
      std::cout << "Path: " << pathObject.filename() << " Ignored" << std::endl;
    }
    return 0;
  }
  return 0;
}

// Debug statement creator for filterer, not fully implemented in the file nor
// with the severity  flag value
void Filterer::debugInfo(std::string info) {
  if (config->at("debug")) {
    std::cout << info << std::endl;
  }
}

/// Main driver for the Filter System
int Filterer::run() {

  std::cout << "starting" << std::endl;

  std::filesystem::path pathObject(configuration.databaseDir);

  std::vector<std::string> filesToFilter = std::vector<std::string>();

  std::cout << "Path: " << pathObject.string() << std::endl;
  /// Check Path exists and get list of files to filter
  int filesFound = getAllCFiles(pathObject, filesToFilter, 0);
  debugInfo("Files Found: " + std::to_string(filesFound));

  /// Loop over all c files in filter list and run through the checker before
  /// creating the AST
  for (std::string fileName : filesToFilter) {
    std::shared_ptr<std::string> contents = std::make_shared<std::string>();
    if (checkPotentialFile(fileName, contents)) {
      std::filesystem::path oldPath(fileName);
      std::filesystem::path newPath(std::filesystem::current_path() /
                                    "filteredFiles");
      /// set up the new path in filteredFiles to keep directory structure
      // prevent writing outside the project directory for now
      for (const std::filesystem::path &component : oldPath) {
        if (component.string() != "..") {
          newPath /= component;
        }
      }

      llvm::outs() << "Setting Up Common Options Parser\n";

      static llvm::cl::OptionCategory myToolCategory("filterer");

      clang::IgnoringDiagConsumer diagConsumer;

      std::string resourceDir = std::getenv("CLANG_RESOURCES");

      /// Set args for AST creation
      std::vector<std::string> args = std::vector<std::string>({
        "clang",
        oldPath.string(),
        "-extra-arg=-fparse-all-comments",
        "-extra-arg=-resource-dir=" + resourceDir,
        "-extra-arg=-Wdocumentation",
        "-extra-arg=-xc",
        "-extra-arg=-I"
      });

      int argc = args.size();

      char** argv = new char*[argc + 1];

      for (int i=0; i<argc; i++) {
        argv[i] = new char[args[i].length() + 1];
        std::strcpy(argv[i], args[i].c_str());
      }

      argv[argc] = nullptr;

      if (argv == nullptr) {
        return 1;
      }

      std::filesystem::create_directories(newPath.parent_path());

      std::error_code ec;
      llvm::raw_fd_ostream output(llvm::StringRef(newPath.string()), ec);

      llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser =
        clang::tooling::CommonOptionsParser::create(argc, (const char **)argv,
                                                    myToolCategory);
      if (!expectedParser) {
        llvm::errs() << expectedParser.takeError();
        return 1;
      }

      clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();

      llvm::outs() << "Building the Tool\n";

      clang::tooling::ClangTool tool(optionsParser.getCompilations(),
                                     optionsParser.getSourcePathList());

      llvm::outs() << "Diagnostic Options\n";

      tool.setDiagnosticConsumer(&diagConsumer);

      llvm::outs() << "Creating Factory\n";

      FrontendFactoryWithArgs factory(config, typesRequested, output);

      llvm::outs() << "Run the Tool\n";

      llvm::outs() << tool.run(&factory) << "\n";

      output.close();

      if (config->at("debug")) {
        std::cout << *contents << std::endl;
      }

      if (config->at("debug") && std::filesystem::exists(newPath)) {
        std::ifstream file(newPath.string());
        std::stringstream buffer;

        if (file.is_open()) {
          buffer << file.rdbuf();
          const std::string fileContents = buffer.str();
          std::cout << fileContents << std::endl;
        } else {
          file.close();
        }
      }
    } else {
      std::cerr << "File: " << fileName << " Does Not Meet Criteria"
        << std::endl;
    }
  }
  return 0;
}
