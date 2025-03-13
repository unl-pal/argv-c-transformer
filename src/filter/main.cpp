#include "include/Filter.h"
#include "include/Remove.h"
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
          /*std::cout << match[2] << std::endl;*/
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
    /*std::cout << count << std::endl;*/
    /*std::cout << badInclude << std::endl;*/
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
                  /*std::shared_ptr<std::vector<std::string>> filesToFilter,*/
                  bool debug = false, int minLoC = 10) {
  if (!std::filesystem::exists(pathObject)) {
    if (debug) {
      /*std::cout << "Path: " << pathObject.filename() << " Does Not Exist"*/
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
      /*std::cout << entry.path() << std::endl;*/
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

/// ============================================================================
/// MAIN FUNCTION SHOULD GO TO DRIVER FUNCTION IN OTHER FILE FOR CALLS WITH FULL
/// ============================================================================
int main(int argc, char** argv) {
  std::cout << "starting" << std::endl;
  bool debug = true;
  if (argc == 2) {
    std::filesystem::path pathObject;
    /*pathObject.assign(std::filesystem::current_path());*/
    pathObject.append(argv[1]);

    /*std::shared_ptr<std::vector<std::string>> filesToFilter(*/
    /*    new std::vector<std::string>);*/
    /*std::shared_ptr<std::vector<std::string>> filesToFilter = std::make_shared<std::vector<std::string>>();*/
    std::vector<std::string> filesToFilter = std::vector<std::string>();

    std::cout << "Path: " << pathObject.string() << std::endl;
    /// Check Path exists and get list of files to filter
    getAllCFiles(pathObject, filesToFilter, debug);

    /// Set args for AST creation
    std::vector<std::string> args = std::vector<std::string>();
    /*args.push_back("-v");*/
    std::vector<std::string> paths = getPathDirectories();
    for (const std::string &dir : paths) {
      args.push_back("-I" + dir);
    }

    std::string indent = "    ";
    /// Loop over all c files in filter list and run through the checker before
    /// creating the AST
    for (std::string fileName : filesToFilter) {
      std::shared_ptr<std::string> contents = std::make_shared<std::string>();
      if (checkPotentialFile(fileName, contents, 10, false)) {
        /// Use args and file content to generate
        std::cout << "Creating astUnit for: " << fileName << std::endl;
        std::unique_ptr<clang::ASTUnit> astUnit =
          clang::tooling::buildASTFromCodeWithArgs(*contents, args, fileName);
        if (debug) {
          std::cout << *contents << std::endl;
        }

        if (!astUnit) {
          std::cout << indent << "AST Unit failed to build" << std::endl;
          break;
        }

        /*std::cout << indent << "Saving AST" << std::endl;*/
        /*astUnit->Save(std::string(fileName) + ".ast");*/

        std::cout << indent << "Diagnostics" << std::endl;
        clang::ASTContext &Context = astUnit->getASTContext();
        astUnit->getDiagnostics();
        Context.PrintStats();

        std::cout << "Creating Counting Visitor" << std::endl;
        CountNodesVisitor countVisitor(&Context);

        std::cout << indent << "Traversing AST" << std::endl;
        std::cout << indent << countVisitor.TraverseAST(Context) << std::endl;
        std::cout << indent << "Printing Report" << std::endl;
        countVisitor.PrintReport(fileName);

        std::cout << indent << "Removing Nodes" << std::endl;
        clang::Rewriter Rewrite;
        Rewrite.setSourceMgr(astUnit->getSourceManager(), astUnit->getLangOpts());
        RemoveFuncVisitor RemoveLameFuncVisitor(&Context, Rewrite, {"doesThing"});
        RemoveLameFuncVisitor.TraverseAST(Context);

        std::cout << indent << "Re-Traversing AST" << std::endl;
        CountNodesVisitor reCountVisitor(&Context);
        reCountVisitor.TraverseAST(Context);
        reCountVisitor.PrintReport(fileName);

        std::string hello = "---------------------------------\n"
          "!! This File Has Been Modified !!\n"
          "---------------------------------\n";
        /*Rewrite.InsertTextBefore(Rewrite.getSourceMgr().getIncludeLoc(Context.getSourceManager().getMainFileID()), hello);*/
        /*auto loc = _C->getSourceManager().getIncludeLoc(_C->getSourceManager().getMainFileID());*/
        /*_R.InsertTextAfterToken(loc, "Hello");*/

        std::cout << "OverWriting" << std::endl;
        Rewrite.setSourceMgr(Context.getSourceManager(), astUnit->getLangOpts());
        std::cout << Rewrite.overwriteChangedFiles() << std::endl;
        /*auto loc = Rewrite.buffer_begin();*/
        /*std::stringstream rewriteBuff;*/
        /*while (loc != Rewrite.buffer_end()) {*/
        /*  const llvm::StringRef line = loc->second.begin().piece();*/
        /*  rewriteBuff << line.str();*/
        /*  loc->second.begin().MoveToNextPiece();*/
        /*}*/

        /*clang::FileID thing;*/
        /*auto *buff = Rewrite.getRewriteBufferFor(thing);*/
        /*std::error_code ec;*/
        /*llvm::raw_fd_ostream output(llvm::StringRef(std::string(argv[1]) + ".i"), ec);*/
        /*auto thing2 = buff->write(output);*/

        if (debug) {
          std::ifstream file(fileName);
          std::stringstream buffer;

          if (file.is_open()) {
            buffer << file.rdbuf();
            file.close();
            const std::string fileContents = buffer.str();
            std::cout << fileContents << std::endl;
          }
        }

        if (!astUnit) {
          std::cerr << "Failed to build AST for: " << fileName << std::endl;
        }

    } else {
      std::cerr << "File: " << fileName << " Does Not Meet Criteria" << std::endl;
      }
    }
  } else {
  std::cout << "Incorrect Number of Args" << std::endl;
  return 1;
  }
}
