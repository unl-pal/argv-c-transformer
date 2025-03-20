#include "include/Filterer.hpp"
#include "include/Filter.h"
#include "include/Remove.h"
#include "include/Utilities.hpp"

#include <clang/Tooling/Tooling.h>
#include <iostream>
#include <regex>
#include <string>

/// vector of all standar library names to compare includes to
/// Checks a file for bad includes, min lines of code and returns false if bad file
/// string pointer holds the results of the file read
bool Filterer::checkPotentialFile(std::string fileName, std::shared_ptr<std::string> contents, int minLoC, bool useBadIncludes) {

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
int Filterer::getAllCFiles(std::filesystem::path pathObject,
			   std::vector<std::string> &filesToFilter,
			   int numFiles,
			   bool debug, int minLoC) {
    if (!std::filesystem::exists(pathObject)) {
	if (debug) {
	    std::cout << "Path: " << " Does Not Exist"
		<< std::endl;
	}
	return 0;
    }
    if (std::filesystem::is_regular_file(pathObject)) {
	if (pathObject.has_extension()) {
	    if (pathObject.extension() == ".c") {
		if (debug) {
		    std::cout << "File: " << pathObject.filename()
			<< " Added To Filter List" << std::endl;
		}
		filesToFilter.push_back(pathObject.string());
		return 1;
	    } else {
		if (debug) {
		    std::cout << "File: " << pathObject.filename() << " is Not a C File"
			<< std::endl;
		}
		return 0;
	    }
	} else {
	    if (debug) {
		std::cout << "File: " << pathObject.filename() << " Has No Extension"
		    << std::endl;
	    }
	    return 0;
	}
    } else if (std::filesystem::is_directory(pathObject)) {
	for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(pathObject)) {
	    return numFiles += getAllCFiles(entry.path(), filesToFilter, debug, minLoC);
	}
    } else {
	if (debug) {
	    std::cout << "Path: " << pathObject.filename() << " Ignored" << std::endl;
	}
	return 0;
    }
}

/// checks the users path for path to libc files for ast generation
std::vector<std::string> Filterer::getPathDirectories() {
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

void Filterer::debugInfo(bool debug, std::string info) {
  if (debug) {
    std::cout << info << std::endl;
  }
}

int Filterer::run(int argc, char** argv) {
    std::cout << "starting" << std::endl;
    bool debug = false;
    if (argc == 2) {
	std::filesystem::path pathObject;
	pathObject.append(argv[1]);

	std::vector<std::string> filesToFilter = std::vector<std::string>();

	std::cout << "Path: " << pathObject.string() << std::endl;
	/// Check Path exists and get list of files to filter
	int filesFound = getAllCFiles(pathObject, filesToFilter, 0, debug, 10);
  debugInfo(debug, "Files Found: " + std::to_string(filesFound));

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
		std::filesystem::path oldPath(fileName);
		std::filesystem::path newPath(std::filesystem::current_path() / "filteredFiles");
		for (const std::filesystem::path &component : oldPath) {
		    if (component.string() != oldPath.begin()->string() && component.string() != "..") {
			newPath /= component;
		    }
		}
		std::filesystem::create_directories(newPath.parent_path());
		std::ofstream filteredFile(newPath.string());
		if (filteredFile.is_open()) {
		    filteredFile << *contents;
		    filteredFile.close();
		} else {
		    std::cout << "Could Not Create Filtered File: " << newPath.string() << std::endl;
		}
		/// Use args and file content to generate
		std::cout << "Creating astUnit for: " << fileName << std::endl;
		std::unique_ptr<clang::ASTUnit> astUnit =
		    clang::tooling::buildASTFromCodeWithArgs(*contents, args, newPath.string());
		if (debug) {
		    std::cout << *contents << std::endl;
		}

		if (!astUnit) {
		    std::cout << indent << "AST Unit failed to build" << std::endl;
		    break;
		}

		clang::ASTContext &Context = astUnit->getASTContext();

		if (debug) {
		    std::cout << indent << "Diagnostics" << std::endl;
		    astUnit->getDiagnostics();
		    Context.PrintStats();
		}

		std::cout << "Main File Name: " << astUnit->getMainFileName().str() << std::endl;
		std::cout << "Creating Counting Visitor" << std::endl;
		CountNodesVisitor countVisitor(&Context);

		std::cout << indent << "Traversing AST" << std::endl;
		std::cout << indent << countVisitor.TraverseAST(Context) << std::endl;

		if (debug) {
		    std::cout << indent << "Printing Report" << std::endl;
		    countVisitor.PrintReport(fileName);
		}

		std::cout << indent << "Removing Nodes" << std::endl;
		clang::Rewriter Rewrite;
		Rewrite.setSourceMgr(astUnit->getSourceManager(), astUnit->getLangOpts());
		RemoveFuncVisitor RemoveLameFuncVisitor(&Context, Rewrite, {"doesThing"});
		RemoveLameFuncVisitor.TraverseAST(Context);

		std::cout << indent << "Re-Traversing AST" << std::endl;
		CountNodesVisitor reCountVisitor(&Context);
		reCountVisitor.TraverseAST(Context);
		if (debug) {
		    reCountVisitor.PrintReport(fileName);
		}

		std::string hello = "---------------------------------\n"
		    "!! This File Has Been Modified !!\n"
		    "---------------------------------\n";

		std::cout << "OverWriting" << std::endl;
		Rewrite.setSourceMgr(Context.getSourceManager(), astUnit->getLangOpts());
		std::cout << Rewrite.overwriteChangedFiles() << std::endl;

		if (debug) {
		    std::ifstream file(newPath.string());
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
    return 0;
}
