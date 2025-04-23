<!--toc:start-->
- [ArgV C Transformer](#argv-c-transformer)
- [Running the Code](#running-the-code)
  - [Dependencies](#dependencies)
    - [Clang Resources](#clang-resources)
  - [Available Targets](#available-targets)
  - [Temporary Scripts](#temporary-scripts)
<!--toc:end-->

# ArgV C Transformer
ArgC transformer takes directories with c files or individual c files and
attempts to convert the potential files into Benchmarks. This is done using
user defined parameters to decide what makes for an interesting file as well as
individual functions.

# Running the Code

Project can be run on folders or individual files as specified by the user.

## Dependencies
- llvm-devel
- clang-devel
- cmake

### Clang Resources
To compile the asts without errors on includes from the c file standard library
headers te clang resource dir must be provided. This can be found by running
`clang -print-resource-dir`

## Available Targets
Filter requres that the user provides the location of the *clang-resource-dir*,
the *dir-to-filter*, and the *propertiesFile* 

Transform requres that the user provides the the location of the
*clang-resource-dir* and the *dir-to-transform*

Full requires that the user provide the *clang-resource-dir*,
*file/dir-to-filter* and the *propertiesFile* location

## Temporary Scripts
2 Scripts are provided with similar functionality and can be modified to suit
user needs as development continues.

- The `run.sh` file runs does the cmake and build steps using ninja as the
generator then clears the old results folders and runs the filter and transform
sequentially with different arguments
- The `fullRun.sh` script runs the full target comprised of all current parts
using the same arguments and default locations
