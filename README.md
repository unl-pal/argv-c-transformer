# Dependencies
llvm-devel
clang-devel

To compile the asts without errors on includes from the c file standard library
headers te clang resource dir must be provided. This can be found by running
`clang -print-resource-dir`

Project can be run on folders or individual files specified by the user.

There is no target for running the full program but the `myRun.sh` script can 
be used as a temporary substitute.

Filter requres that the user provides the *sourceDir*, *propertiesFile* and the location of the clang resource directory.

Transform requres that the user provides the *sourceDir* and the location of the clang resource directory.
