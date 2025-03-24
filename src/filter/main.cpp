#include "include/Filterer.hpp"
#include "include/Utilities.hpp"

#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/Tooling.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <regex>

/// vector of all standar library names to compare includes to
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
    "wctype.h", 
  };

/// Checks a file for bad includes, min lines of code and returns false if bad file
/// string pointer holds the results of the file read
bool checkPotentialFile(std::string fileName, std::shared_ptr<std::string> contents, int minLoC, bool useBadIncludes = false) {

  std::ifstream file(fileName);
  std::stringstream buffer;

  if (file.is_open()) {
    std::regex pattern("#(include|import)\\s[<\"]([\\w/\\.]*)[\">]");
    std::string line;
    std::smatch match;
    int count = 0;
    while (std::getline(file, line)) {
      if (std::regex_search(line, match, pattern)) {
        if (std::find(stdLibNames.begin(), stdLibNames.end(), match[2]) !=
          stdLibNames.end()) {
          std::cout << match[2] << std::endl;
        } else if (!useBadIncludes) {
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
    if (count < minLoC) {
      return false;
    } else {
      *contents = buffer.str();
      return true;
    }
  } else {
    std::cerr << "File Failed to Open" << std::endl;
    return false;
  }
}

/// Looks for all c files in a directory structure and adds to vector
/// returns bool as place holder, has little meaning at the moment and should be fixed
bool getAllCFiles(std::filesystem::path pathObject,
                  std::vector<std::string> &filesToFilter,
                  bool debug = false, int minLoC = 10) {
  if (!std::filesystem::exists(pathObject)) {
    if (debug) {
      std::cout << "Path: " << " Does Not Exist"
                << std::endl;
    }
    return false;
  }
  if (std::filesystem::is_regular_file(pathObject)) {
    if (pathObject.has_extension()) {
      if (pathObject.extension() == ".c") {
        if (debug) {
          std::cout << "File: " << pathObject.filename()
                    << " Added To Filter List" << std::endl;
        }
        filesToFilter.push_back(pathObject.string());
        return true;
      } else {
        if (debug) {
          std::cout << "File: " << pathObject.filename() << " is Not a C File"
                    << std::endl;
        }
        return false;
      }
    } else {
      if (debug) {
        std::cout << "File: " << pathObject.filename() << " Has No Extension"
                  << std::endl;
      }
      return false;
    }
  } else if (std::filesystem::is_directory(pathObject)) {
    bool result = true;
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(pathObject)) {
      result &= getAllCFiles(entry.path(), filesToFilter, debug, minLoC);
    }
    return result;
  } else {
    if (debug) {
      std::cout << "Path: " << pathObject.filename() << " Ignored" << std::endl;
    }
    return false;
  }
}

/// checks the users path for path to libc files for ast generation
std::vector<std::string> getPathDirectories() {
  std::vector<std::string> directories;
  const char* pathEnv = std::getenv("PATH");
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

/// Target for calling the Filterer Individually
int main(int argc, char** argv) {
  if (argc == 3) {
    Filterer filter;
    filter.run(argc, argv);

  } else {
  std::cout << "Incorrect Number of Args" << std::endl;
  std::cout << "Please Give the Location of the File or Directory to Filter "
               "and the Location of the Configuration File\n"
               "Example: `<filter> <directory> <config-file>`"
            << std::endl;
  return 1;
  }
}
