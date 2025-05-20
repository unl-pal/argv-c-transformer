# ArgV C Transformer

<!--toc:start-->
- [ArgV C Transformer](#argv-c-transformer)
- [Running the Code](#running-the-code)
  - [Dependencies](#dependencies)
    - [Clang Resources](#clang-resources)
  - [Available Targets](#available-targets)
  - [Temporary Scripts](#temporary-scripts)
<!--toc:end-->

ArgC transformer takes directories with c files or individual c files and
attempts to convert the potential files into Benchmarks. This is done using
user defined parameters to decide what makes for an interesting file as well as
individual functions.

## Dependencies

To develop and run the code the following are needed:

- llvm-devel
- clang-devel
- cmake
- make or ninja
- a c++ compiler

### Clang Resources

To compile the ASTs without errors on includes from the c file standard library
headers, the clang resource dir must be provided to the compiler. This is done
via an environment variable called `CLANG_RESOURCES`. This must be set by the
user on the system. The location of the resource directory can be found by
running `clang -print-resource-dir`.

## Running the Code

The project can be run on folders or individual files as specified by the user.
The attributes used to filter out undesired functions are set via the
config.properties file. To run any of the targets the user must first run the
cmake command followed by the make or ninja command, depending on the generator
chosen. Examples of these commands on a unix system would look something like these.
If generating from inside the build folder:

```sh
cd build
cmake ..
make
cd ..
./build/<TARGET> <args>
```

If generating from the root dir:

```sh
cmake -B build -S .
make -C build
./build/<TARGET> <args>
```

Other Arguments can be added to change the behavior. To set the cmake Generator
to Ninja the user would add the `-G Ninja` argument. To adjust the number of
processors used by the generator the `-j<N>` argument can be used. For example:

```sh
cmake -B build -S . -G Ninja
ninja -C build -j2
./build/<TARGET> <args>
```

Would set the generator to ninja then use 2 processors to compile, then run the
chosen target.

## Available Targets

Filter requres that the user provides the location of the *dir-to-filter*, and
the *propertiesFile*

Transform requres that the user provides the the location of the *dir-to-transform*

Full requires that the user provide the
*file/dir-to-filter* and the *propertiesFile* location

## Temporary Scripts

2 Scripts are provided with similar functionality and can be modified to suit
user needs as development continues.

- The `run.sh` file runs does the cmake and build steps using ninja as the
generator then clears the old results folders and runs the filter and transform
sequentially with the necessary arguments. It also copies the compile_commands.json
file to the root dir. This file is used by many ide's for linting.
- The `fullRun.sh` script runs the full target comprised of all current parts
using the same arguments and default locations

There is also a `clean.sh` script provided that cleans the build folder without
destroying the dependencies.

These scripts can act as a template for user scripts or as an example on how to
run the code on unix based systems.
