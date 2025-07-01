# ArgV C Transformer

<!--toc:start-->
- [ArgV C Transformer](#argv-c-transformer)
  - [Dependencies](#dependencies)
    - [Clang Resources](#clang-resources)
  - [Running the Code](#running-the-code)
  - [Targets](#targets)
  - [Scripts](#scripts)
    - [For Running on Sample Code](#for-running-on-sample-code)
    - [For Running on Live Code](#for-running-on-live-code)
    - [For Cleaning In Between Major Changes](#for-cleaning-in-between-major-changes)
<!--toc:end-->

ArgC transformer takes directories with c files or individual c files and
attempts to convert the potential files into Benchmarks. This is done using
user defined parameters to decide what makes for an interesting file as well as
individual functions.

## Dependencies

To develop and run the code the following are needed:

- llvm developer toolkit
- clang and the clang developer toolkit
- cmake
- make or ninja
- a c++ compiler

### Clang Resources

To compile the ASTs without errors on includes from the c file standard library
headers, the clang resource dir must be provided to the compiler. This is done
via an environment variable called `CLANG_RESOURCES`. This must be set by the
user on the system. The location of the resource directory can be found by
running `clang -print-resource-dir`. The code cannot be run until this Variable
has been set.

## Running the Code

The project can be run on folders or individual files as specified by the user
in the provided configuration file. The configuration file also provides all
settings used by the filter and the transformer during the run. Such as the
minimum number of loops needed or location of the filtered files.

The name of the configuration file is passed to the program as a commandline
argument and can be passed to a tool individually or, if using either the
'run.sh' or 'liveRun.sh' scripts, to the program as a whole as the first and
only argument.

To run any of the targets the user must first run the
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

This would set the generator to ninja then use 2 processors to compile, then
runs the chosen target.

## Targets

Filter requres that the user provides the location of the *configurationFile*

Transform requres that the user provides the the location of the *configurationFile*

Full requires that the user provide the
*file/dir-to-filter* and the *propertiesFile* location

## Scripts

All scripts currently only exist for unix based systems. These can be adapted
or used as a template for other systems if desired by the user but all changed
scripts should remain local to the user's system.

### For Running on Sample Code

- `run.sh <config-file-name>`:
  - runs cmake from root using Ninja as the Generator
  - clears all old results folders
  - runs the filter and transform sequentially with specified config file
  - It also copies the compile_commands.json file to the root dir (This file is
  used by many ide's for linting)

### For Running on Live Code

- `liveRun.sh <config-file-name>`
  - runs cmake from root using Ninja as the Generator
  - clears all old results folders
  - runs the downloader pulling live repositories from github
    - the repos pulled are found in the specified csv file and are filtered by
    the downloader
  - runs the filter and transform sequentially with specified config file
  - It also copies the compile_commands.json file to the root dir (This file is
  used by many ide's for linting)

### For Cleaning In Between Major Changes

- `clean.sh`
  - script provided that cleans the build folder without destroying the dependencies.

These scripts can act as a template for user scripts or as an example on how to
run the code on unix based systems.
