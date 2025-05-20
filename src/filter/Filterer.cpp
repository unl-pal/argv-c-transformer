#include "include/Filterer.hpp"
#include "include/Remove.hpp"

#include <clang/AST/Type.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/Tooling.h>
#include <fstream>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <regex>

Filterer::Filterer(){
  typesRequested = std::vector<unsigned int>();
  typeNames = std::vector<std::string>();
};

// Parse the config file for all settings as well as a list of desired types
void Filterer::parseConfigFile(std::string configFile) {
  std::ifstream file(configFile);
  if (!std::filesystem::exists(configFile)) {
    std::cout << "File: " << configFile << " Does Not Exist" << std::endl;
    std::cout << "Using Default Settings" << std::endl;
    return;
  }
  if (file.is_open()) {
    std::cout << "Using: " << configFile << " Specified Settings" << std::endl;
    std::regex pattern("^\\s*(\\w+)\\s*=\\s*([0-9]+|[\\w\\s,]+)$");
    std::string line;
    std::smatch match;
    while (std::getline(file, line)) {
      if (std::regex_search(line, match, pattern)) {
        std::string key = match[1];
        std::string value = match[2];
        // Add the value to the config if the key is a member of the map
        if (config.count(key)) {
          try {
            int i = std::stoi(value);
            config[key] = i;
          } catch (...) {
          } 
          // For true false values convert to 1s and 0s
          if (value == "false" || value == "False") {
            config[key] = 0;
          } else if (value == "true" || value == "True") {
            config[key] = 1;
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
        }
        else {
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
    std::cout << "For Types: " << std::endl;
    for (std::string T : typeNames) {
      std::cout << T << std::endl;
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
      *contents = "";
      return false;
    } else if (count > config["maxFileLoC"]) {
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

// Checks that the function has all necessary properties to continue or adds to
// the list of functions to remove
std::vector<std::string> Filterer::filterFunctions(
  std::unordered_map<std::string, CountNodesVisitor::attributes *> functions) {
  std::vector<std::string> functionsToRemove;
  for (std::pair<std::string, CountNodesVisitor::attributes*> func : functions) {
    std::string key = func.first;
    CountNodesVisitor::attributes *attr = func.second;
    if (key == "Program" || key == "main") {
      continue;
    } else if (attr->ForLoops > config["maxForLoops"]) {
      functionsToRemove.push_back(key);
    } else if (attr->WhileLoops > config["maxWhileLoops"]) {
      functionsToRemove.push_back(key);
    } else if (attr->CallFunc > config["maxCallFunc"]) {
      functionsToRemove.push_back(key);
    } else if (attr->Functions > config["maxFunctions"]) {
      functionsToRemove.push_back(key);
    } else if (attr->IfStmt > config["maxIfStmt"]) {
      functionsToRemove.push_back(key);
    } else if (attr->Param > config["maxParam"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeArithmeticOperation > config["maxTypeArithmeticOperation"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeCompareOperation > config["maxTypeCompareOperation"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeComparisons > config["maxTypeComparisons"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeIfStmt > config["maxTypeIfStmt"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeParameters > config["maxTypeParameters"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypePostfix > config["maxTypePostfix"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypePrefix > config["maxTypePrefix"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeUnaryOperation > config["maxTypeUnaryOperation"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeVariableReference > config["maxTypeVariableReference"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeVariables > config["maxTypeVariables"]) {
      functionsToRemove.push_back(key);
    } else if (attr->ForLoops < config["minForLoops"]) {
      functionsToRemove.push_back(key);
    } else if (attr->CallFunc < config["minCallFunc"]) {
      functionsToRemove.push_back(key);
    } else if (attr->ForLoops < config["minForLoops"]) {
      functionsToRemove.push_back(key);
    } else if (attr->Functions < config["minFunctions"]) {
      functionsToRemove.push_back(key);
    } else if (attr->IfStmt < config["minIfStmt"]) {
      functionsToRemove.push_back(key);
    } else if (attr->Param < config["minParam"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeArithmeticOperation < config["minTypeArithmeticOperation"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeCompareOperation < config["minTypeCompareOperation"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeComparisons < config["minTypeComparisons"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeIfStmt < config["minTypeIfStmt"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeParameters < config["minTypeParameters"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypePostfix < config["minTypePostfix"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypePrefix < config["minTypePrefix"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeUnaryOperation < config["minTypeUnaryOperation"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeVariableReference < config["minTypeVariableReference"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeVariables < config["minTypeVariables"]) {
      functionsToRemove.push_back(key);
    } else if (attr->WhileLoops < config["minWhileLoops"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeCompareOperation < config["minTypeCompareOperation"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeArithmeticOperation < config["minTypeArithmeticOperation"]) {
      functionsToRemove.push_back(key);
    } else if (attr->TypeIfStmt < config["minTypeIfStmt"]) {
      functionsToRemove.push_back(key);
    }
  }
  return functionsToRemove;
}

// Debug statement creator for filterer, not fully implemented in the file nor
// with the severity  flag value
void Filterer::debugInfo(std::string info) {
  if (config["debug"]) {
    std::cout << info << std::endl;
  }
}

/// Main driver for the Filter System
int Filterer::run(std::string fileOrDirToFilter,
                  std::string propertiesConfigFile) {

  std::cout << "starting" << std::endl;

    parseConfigFile(propertiesConfigFile);

    std::filesystem::path pathObject(fileOrDirToFilter);

    std::vector<std::string> filesToFilter = std::vector<std::string>();

    std::cout << "Path: " << pathObject.string() << std::endl;
    /// Check Path exists and get list of files to filter
    int filesFound = getAllCFiles(pathObject, filesToFilter, 0);
    debugInfo("Files Found: " + std::to_string(filesFound));

    /// Set args for AST creation
    std::vector<std::string> args = std::vector<std::string>();
    // This ensures comments are a part of the parsed AST
    args.push_back("-fparse-all-comments");
    // This tells the AST generator where to find the compiler supplied headers
    args.push_back(std::string("-resource-dir=") + std::string(std::getenv("CLANG_RESOURCES")));

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
          std::cout << "Diagnostics" << std::endl;
          astUnit->getDiagnostics();
          Context.PrintStats();
        }

        std::cout << "Main File Name: " << astUnit->getMainFileName().str()
                  << std::endl;
        std::cout << "Creating Counting Visitor" << std::endl;
        CountNodesVisitor countVisitor(&Context, typesRequested);

        std::cout << "Traversing AST" << std::endl;
        countVisitor.TraverseAST(Context);

        if (config["debug"]) {
          std::cout << "Printing Report" << std::endl;
          countVisitor.PrintReport(fileName);
        }

        std::unordered_map<std::string, CountNodesVisitor::attributes*> functions = countVisitor.ReportAttributes();
        std::vector<std::string> functionsToRemove = filterFunctions(functions);

        // If all funtions besides the global 'function', Program, which holds
        // all variables and declarations made outside of functions are removed
        // then do not add the file
        if (functions.size() && functions.size() - functionsToRemove.size() <= 1) {
          std::cout << "No Potential Funtions In: " << fileName << std::endl;
          std::cout << "Moving to Next Potential File" << std::endl;
          continue;
        }

        std::filesystem::create_directories(newPath.parent_path());

        clang::Rewriter Rewrite;
        Rewrite.setSourceMgr(Context.getSourceManager(),
                             astUnit->getLangOpts());
        if (functionsToRemove.size()) {
          std::cout << "Removing Nodes\n";
          for (std::string node : functionsToRemove) {
            std::cout << node + "\n";
          }
          std::cout << std::endl;
          RemoveFuncVisitor RemoveFunctionsVisitor(&Context, Rewrite,
                                                   functionsToRemove);
        // RemoveFunctionsVisitor.TraverseAST(Context);
        // TODO run for each function name may be the way to go...
        for (std::string name : functionsToRemove) {
          RemoveFunctionsVisitor.TraverseAST(Context);
        }
        }

        std::cout << "Writing File" << std::endl;
        std::error_code ec;
        llvm::raw_fd_ostream output(llvm::StringRef(newPath.string()), ec);
        Rewrite.getEditBuffer(Context.getSourceManager().getFileID(astUnit->getStartOfMainFileID())).write(output);
        std::cout << "Finished Rewrite step" << std::endl;


        if (config["debug"] && std::filesystem::exists(newPath)) {
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
