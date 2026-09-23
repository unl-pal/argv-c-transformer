# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

if(APPLE)
  set(LLVM_HOMEBREW_NAMES llvm llvm@22 llvm@21 llvm@20)
  foreach(brew_prefix /opt/homebrew/opt /usr/local/opt)
    foreach(llvm_name ${LLVM_HOMEBREW_NAMES})
      if(EXISTS "${brew_prefix}/${llvm_name}")
        set(LLVM_HOMEBREW_PREFIX "${brew_prefix}/${llvm_name}")
        break()
      endif()
    endforeach()
    if(LLVM_HOMEBREW_PREFIX)
      break()
    endif()
  endforeach()
  if(NOT LLVM_HOMEBREW_PREFIX)
    message(FATAL_ERROR "No Homebrew LLVM found (looked for: ${LLVM_HOMEBREW_NAMES}). Install with: brew install llvm")
  endif()

  set(CMAKE_CXX_COMPILER "${LLVM_HOMEBREW_PREFIX}/bin/clang++")
  set(CMAKE_C_COMPILER "${LLVM_HOMEBREW_PREFIX}/bin/clang")
  list(APPEND CMAKE_PREFIX_PATH "${LLVM_HOMEBREW_PREFIX}")
elseif(NOT DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX})
  # This project links libclang-cpp/libLLVM and compiles against Clang's C++
  # headers directly, so it must be built with clang, not whatever CMake's
  # default search would otherwise pick (e.g. GCC on most Linux distros).
  find_program(ARGV_C_CLANGXX NAMES clang++-22 clang++-21 clang++-20 clang++)
  find_program(ARGV_C_CLANG NAMES clang-22 clang-21 clang-20 clang)
  if(ARGV_C_CLANGXX AND ARGV_C_CLANG)
    set(CMAKE_CXX_COMPILER "${ARGV_C_CLANGXX}")
    set(CMAKE_C_COMPILER "${ARGV_C_CLANG}")
  else()
    message(WARNING "No clang found on PATH. This project requires clang/LLVM "
      "20+ and will likely fail to build with CMake's default compiler. "
      "Install clang or point cmake at one: "
      "CXX=clang++ CC=clang cmake -B build -S .")
  endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
  argv-argc-transformer
  VERSION 0.0.0
  LANGUAGES C CXX
)

find_package(LLVM REQUIRED)
if(LLVM_PACKAGE_VERSION VERSION_LESS "20")
  message(FATAL_ERROR "LLVM 20 required, found ${LLVM_PACKAGE_VERSION}")
endif()

link_directories(${LLVM_LIBRARY_DIRS})

set(LLVM_LIBS LLVM LLVMSupport clang-cpp clang)

set(HEADERS_FILTER src/filter/include)
set(HEADERS_TRANSFORM src/transform/include)
set(HEADERS_VERIFY src/verify/include)
set(HEADERS_COMMON src/common/include)

# Embed src/common/argv_c_harness.h into the binary at build time, so
# Verifier can write it into each benchmarkDir without any runtime lookup of
# an installed resource file. Edit the .h, not the generated header.
# file(READ) alone creates no build dependency, so CMAKE_CONFIGURE_DEPENDS is
# required - without it, editing the .h wouldn't trigger a reconfigure and
# `ninja` would silently keep embedding stale content.
set(ARGV_C_HARNESS_HEADER "${CMAKE_SOURCE_DIR}/src/common/argv_c_harness.h")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ARGV_C_HARNESS_HEADER}")
file(READ "${ARGV_C_HARNESS_HEADER}" ARGV_C_HARNESS_HEADER_CONTENT)
set(HEADERS_GENERATED "${CMAKE_BINARY_DIR}/generated")
configure_file(
  "${HEADERS_COMMON}/HarnessHeaderData.hpp.in"
  "${HEADERS_GENERATED}/HarnessHeaderData.hpp"
  @ONLY
)

set(SOURCE_FILTER
  src/filter/CountingConsumer.cpp
  src/filter/CountingVisitor.cpp
  src/filter/FilterAction.cpp
  src/filter/Filterer.cpp
  src/filter/FilterFunctionsConsumer.cpp
  src/filter/HeaderClosure.cpp
  src/filter/RemoveConsumer.cpp
  src/filter/RemoveVisitor.cpp
)

set(SOURCE_TRANSFORM
  src/transform/AddStdIncludesConsumer.cpp
  src/transform/MainGenConsumer.cpp
  src/transform/HavocCallsVisitor.cpp
  src/transform/HavocCallsConsumer.cpp
  src/transform/TransformAction.cpp
  src/transform/Transformer.cpp
  src/transform/UnknownTypeDiagConsumer.cpp
)

set(SOURCE_VERIFY
  src/verify/HarnessRepairConsumer.cpp
  src/verify/Verifier.cpp
  src/verify/VerifyAction.cpp
  src/verify/VerifyFunctionsConsumer.cpp
)

# filter_lib / transform_lib / verify_lib let the test binaries link the
# pipelines without pulling in main()
add_library(filter_lib STATIC ${SOURCE_FILTER})
target_include_directories(filter_lib SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_FILTER} ${HEADERS_COMMON})
target_link_libraries(filter_lib PRIVATE ${LLVM_LIBS})

add_library(transform_lib STATIC ${SOURCE_TRANSFORM})
target_include_directories(transform_lib SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_TRANSFORM} ${HEADERS_COMMON} ${HEADERS_GENERATED})
target_link_libraries(transform_lib PRIVATE ${LLVM_LIBS})

# verify reuses the filter's counting/removal consumers, hence HEADERS_FILTER
# and the filter_lib link
add_library(verify_lib STATIC ${SOURCE_VERIFY})
target_include_directories(verify_lib SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_VERIFY} ${HEADERS_FILTER} ${HEADERS_COMMON} ${HEADERS_GENERATED})
target_link_libraries(verify_lib PRIVATE filter_lib ${LLVM_LIBS})

add_executable(filter src/filter/main.cpp)
add_executable(transform src/transform/main.cpp)
add_executable(verify src/verify/main.cpp)
# full binary that runs the entire pipeline:
add_executable(argv-c src/full/main.cpp)

target_include_directories(filter SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_FILTER} ${HEADERS_COMMON})
target_include_directories(transform SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_TRANSFORM} ${HEADERS_COMMON})
target_include_directories(verify SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_VERIFY} ${HEADERS_FILTER} ${HEADERS_COMMON})
target_include_directories(argv-c SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_VERIFY} ${HEADERS_TRANSFORM} ${HEADERS_FILTER} ${HEADERS_COMMON})

target_link_libraries(filter PRIVATE filter_lib ${LLVM_LIBS})
target_link_libraries(transform PRIVATE transform_lib ${LLVM_LIBS})
target_link_libraries(verify PRIVATE verify_lib filter_lib ${LLVM_LIBS})
target_link_libraries(argv-c PRIVATE filter_lib transform_lib verify_lib ${LLVM_LIBS})

if(UNIX)
  set_property(TARGET filter PROPERTY LINKER_TYPE LLD)
  set_property(TARGET transform PROPERTY LINKER_TYPE LLD)
  set_property(TARGET verify PROPERTY LINKER_TYPE LLD)
  set_property(TARGET argv-c PROPERTY LINKER_TYPE LLD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused -fcolor-diagnostics")
endif()

enable_testing()
add_subdirectory(tests)

# Only argv-c is meant for a normal user's install
install(TARGETS argv-c)
