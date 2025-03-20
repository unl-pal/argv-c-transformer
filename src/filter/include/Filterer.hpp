#include <filesystem>
#include <memory>
#include <string>
#include <vector>

/// Checks a file for bad includes, min lines of code and returns false if bad file
/// string pointer holds the results of the file read
class Filterer {
public:
    bool checkPotentialFile(std::string fileName, std::shared_ptr<std::string> contents, int minLoC, bool useBadIncludes = false);

    /// Looks for all c files in a directory structure and adds to vector
    /// returns bool as place holder, has little meaning at the moment and should be fixed
    int getAllCFiles(std::filesystem::path pathObject,
                      std::vector<std::string> &filesToFilter,
                      int numFiles = 0,
                      bool debug = false, int minLoC = 10);
    ///
    /// checks the users path for path to libc files for ast generation
    std::vector<std::string> getPathDirectories();

    void debugInfo(bool debug, std::string info);

    int run(int argc, char** argv);

private:
    /// vector of all standar library names to compare includes to
    const std::vector<std::string> stdLibNames =
    {   "assert.h",
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
        "wctype.h"
    };
};
