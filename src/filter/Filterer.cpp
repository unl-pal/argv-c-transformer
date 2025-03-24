#include "include/Filterer.hpp"

#include "Filter.h"
#include "include/Remove.h"
#include "include/Utilities.hpp"

#include <clang/Tooling/Tooling.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <utility>

Filterer::Filterer(){};

void Filterer::parseConfigFile(std::string configFile) {
  std::ifstream file(configFile);
  if (!std::filesystem::exists(configFile)) {
    std::cout << "File: " << configFile << " Does Not Exist" << std::endl;
    std::cout << "Using Default Settings" << std::endl;
    return;
  }
  if (file.is_open()) {
    std::cout << "Using: " << configFile << " Specified Settings" << std::endl;
    std::regex pattern("\\ *(\\w+)\\ *=\\ *([0-9]+|\\w+)");
    std::string line;
    std::smatch match;
    while (std::getline(file, line)) {
      if (std::regex_search(line, match, pattern)) {
        std::string key = match[1];
        if (config.count(key)) {
          std::string value = match[2];
          try {
            int i = std::stoi(value);
            config[key] = i;
          } catch (...) {
          } if (value == "false" || value == "False") {
            config[key] = 0;
          } else if (value == "true" || value == "True") {
            config[key] = 1;
          }
        } else {
          std::cout << "Key: " << key
                    << " Is Not A Valid Key For Filtering Files" << std::endl;
        }
      }
    }
    file.close();
    std::cout << "Using Config Settings:" << std::endl;
    for (std::pair item : config) {
      std::cout << "Property: " << item.first << "=" << item.second << std::endl;
    }
  } else {
    std::cerr << "File Failed to Open" << std::endl;
    std::cout << "Using Default Settings" << std::endl;
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
        } else if (!config["useNonStdHeaders"]) {
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
    if (count < config["minFileLoC"]) {
      return false;
    } else if (count > config["maxFileLoC"]) {
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

int Filterer::getAllCFiles(std::filesystem::path pathObject,
                           std::vector<std::string> &filesToFilter,
                           int numFiles) {
  if (!std::filesystem::exists(pathObject)) {
    if (config["debug"]) {
      std::cout << "Path: " << " Does Not Exist" << std::endl;
    }
    return 0;
  }
  if (std::filesystem::is_regular_file(pathObject)) {
    if (pathObject.has_extension()) {
      if (pathObject.extension() == ".c") {
        if (config["debug"]) {
          std::cout << "File: " << pathObject.filename()
                    << " Added To Filter List" << std::endl;
        }
        filesToFilter.push_back(pathObject.string());
        return 1;
      } else {
        if (config["debug"]) {
          std::cout << "File: " << pathObject.filename() << " is Not a C File"
                    << std::endl;
        }
        return 0;
      }
    } else {
      if (config["debug"]) {
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
    if (config["debug"]) {
      std::cout << "Path: " << pathObject.filename() << " Ignored" << std::endl;
    }
    return 0;
  }
  return 0;
}

std::vector<std::string> Filterer::filterFunctions(
  std::unordered_map<std::string, CountNodesVisitor::attributes *> functions) {
  std::vector<std::string> functionsToRemove;
  for (std::pair<std::string, CountNodesVisitor::attributes*> func : functions) {
    std::string key = func.first;
    CountNodesVisitor::attributes *attr = func.second;
    if (key == "Program") {
      continue;
    } else if (attr->numIfStmt < config["minNumIfStmt"]) {
      functionsToRemove.push_back(key);
    } else if (attr->numLoopFor > config["maxNumLoopFor"]) {
      functionsToRemove.push_back(key);
    } else if (attr->numLoopWhile > config["maxNumLoopWhile"]) {
      functionsToRemove.push_back(key);
    } else if (attr->numVarRefInt < config["minNumVarRefInt"]) {
      functionsToRemove.push_back(key);
    } else if (attr->numOpCompare < config["minNumOpCompare"]) {
      functionsToRemove.push_back(key);
    }
  }
  return functionsToRemove;
}

/// checks the users path for path to libc files for ast generation
std::vector<std::string> Filterer::getPathDirectories() {
  std::vector<std::string> directories;
  const char *pathEnv = std::getenv("PATH");
  if (pathEnv != nullptr) {
    std::string pathString(pathEnv);
    std::stringstream ss(pathString);
    std::string token;
    char delimiter = ':';
#ifdef _WIN32
    delimiter = ';';
#endif
    while (std::getline(ss, token, delimiter)) {
      directories.push_back(token);
    }
  }
  return directories;
}

void Filterer::debugInfo(std::string info) {
  if (config["debug"]) {
    std::cout << info << std::endl;
  }
}

/// Main driver for the Filter System
int Filterer::run(int argc, char **argv) {
  std::cout << "starting" << std::endl;
  if (argc == 3) {
    parseConfigFile(argv[2]);

    std::filesystem::path pathObject(argv[1]);

    std::vector<std::string> filesToFilter = std::vector<std::string>();

    std::cout << "Path: " << pathObject.string() << std::endl;
    /// Check Path exists and get list of files to filter
    int filesFound = getAllCFiles(pathObject, filesToFilter, 0);
    debugInfo("Files Found: " + std::to_string(filesFound));

    /// Set args for AST creation
    /// including path to c standard headers from user path
    std::vector<std::string> args = std::vector<std::string>();
    std::vector<std::string> paths = getPathDirectories();
    for (const std::string &dir : paths) {
      args.push_back("-I" + dir);
    }

    // string indent to use for organizing debug statements
    std::string indent = "    "; // TODO something about this, is it needed?
    std::string hello = "// ---------------------------------\n"
                        "// !! This File Has Been Modified !!\n"
                        "// ---------------------------------\n";

    /// Loop over all c files in filter list and run through the checker before
    /// creating the AST
    for (std::string fileName : filesToFilter) {
      std::shared_ptr<std::string> contents = std::make_shared<std::string>();
      *contents += hello;
      if (checkPotentialFile(fileName, contents)) {
        std::filesystem::path oldPath(fileName);
        std::filesystem::path newPath(std::filesystem::current_path() /
                                      "filteredFiles");
        /// set up the new path in filteredFiles to keep directory structure
        for (const std::filesystem::path &component : oldPath) {
          if (component.string() != oldPath.begin()->string() &&
              component.string() != "..") {
            newPath /= component;
          }
        }

        /// Use args and file content to generate
        std::cout << "Creating astUnit for: " << fileName << std::endl;
        std::unique_ptr<clang::ASTUnit> astUnit =
          clang::tooling::buildASTFromCodeWithArgs(*contents, args,
                                                   newPath.string());
        if (config["debug"]) {
          std::cout << *contents << std::endl;
        }

        if (!astUnit) {
          std::cerr << "Failed to build AST for: " << fileName << std::endl;
          continue;
        }

        clang::ASTContext &Context = astUnit->getASTContext();

        if (config["debug"]) {
          std::cout << indent << "Diagnostics" << std::endl;
          astUnit->getDiagnostics();
          Context.PrintStats();
        }

        std::cout << "Main File Name: " << astUnit->getMainFileName().str()
                  << std::endl;
        std::cout << "Creating Counting Visitor" << std::endl;
        CountNodesVisitor countVisitor(&Context);

        std::cout << indent << "Traversing AST" << std::endl;
        countVisitor.TraverseAST(Context);

        if (config["debug"]) {
          std::cout << indent << "Printing Report" << std::endl;
          countVisitor.PrintReport(fileName);
        }

        std::unordered_map<std::string, CountNodesVisitor::attributes*> functions = countVisitor.ReportAttributes();
        std::vector<std::string> functionsToRemove = filterFunctions(functions);

        // If all funtions besides the global 'function', Program, which holds
        // all variables and declarations made outside of functions are removed
        // then do not add the file
        if (functions.size() - functionsToRemove.size() <= 1) {
          std::cout << "No Potential Funtions In: " << fileName << std::endl;
          std::cout << "Moving to Next Potential File" << std::endl;
          continue;
        }

        std::filesystem::create_directories(newPath.parent_path());

        std::cout << indent << "Removing Nodes" << std::endl;
        clang::Rewriter Rewrite;
        Rewrite.setSourceMgr(astUnit->getSourceManager(),
                             astUnit->getLangOpts());
        RemoveFuncVisitor RemoveFunctionsVisitor(&Context, Rewrite,
                                                functionsToRemove);
        RemoveFunctionsVisitor.TraverseAST(Context);

        std::cout << "Writing File" << std::endl;
        Rewrite.setSourceMgr(Context.getSourceManager(),
                             astUnit->getLangOpts());
        std::cout << Rewrite.overwriteChangedFiles() << std::endl;


        if (config["debug"]) {
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
  } else {
    std::cout << "Incorrect Number of Args" << std::endl;
    return 1;
  }
  return 0;
}
