# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

if(APPLE)
  if(EXISTS /opt/homebrew/opt/llvm@20)
    set(LLVM_HOMEBREW_PREFIX /opt/homebrew/opt/llvm@20)
  elseif(EXISTS /usr/local/opt/llvm@20)
    set(LLVM_HOMEBREW_PREFIX /usr/local/opt/llvm@20)
  else()
    message(FATAL_ERROR "llvm@20 not found (expected Homebrew llvm@20). Install with: brew install llvm@20")
  endif()

  set(CMAKE_CXX_COMPILER "${LLVM_HOMEBREW_PREFIX}/bin/clang++")
  set(CMAKE_C_COMPILER "${LLVM_HOMEBREW_PREFIX}/bin/clang")
  list(APPEND CMAKE_PREFIX_PATH "${LLVM_HOMEBREW_PREFIX}")
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
