# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
#
# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
  URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
# Prevent GTest from overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# GTest defaults to installing its own headers/libs alongside the project's -
# it's a test-only dependency and has no business in a user's `cmake --install`.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(filter_tests
  common/ClangToolUtilsTest.cpp
  common/ConfigParserTest.cpp
  common/HavocPolicyTest.cpp
  common/IncludeIndexTest.cpp
  common/WorkerPoolTest.cpp
  filter/CountingVisitorTest.cpp
  filter/FilterFunctionsConsumerTest.cpp
  filter/FiltererStageTest.cpp
)

target_include_directories(filter_tests SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
)
target_include_directories(filter_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/filter/include
  ${CMAKE_SOURCE_DIR}/src/filter
  ${CMAKE_SOURCE_DIR}/src/common/include
)

target_link_libraries(filter_tests PRIVATE
  filter_lib
  ${LLVM_LIBS}
  GTest::gtest_main
)

add_executable(transform_tests transform/TransformGoldenTest.cpp)

target_include_directories(transform_tests SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
)
target_include_directories(transform_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/transform/include
  ${CMAKE_SOURCE_DIR}/src/transform
  ${CMAKE_SOURCE_DIR}/src/common/include
)
# Where the golden .input.c / .expected.c pairs live
target_compile_definitions(transform_tests PRIVATE
  TRANSFORM_TEST_CASES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/transform/cases"
)

target_link_libraries(transform_tests PRIVATE
  transform_lib
  ${LLVM_LIBS}
  GTest::gtest_main
)

add_executable(transform_stage_tests transform/TransformStageTest.cpp)

target_include_directories(transform_stage_tests SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
)
target_include_directories(transform_stage_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/transform/include
  ${CMAKE_SOURCE_DIR}/src/transform
  ${CMAKE_SOURCE_DIR}/src/common/include
)

target_link_libraries(transform_stage_tests PRIVATE
  transform_lib
  ${LLVM_LIBS}
  GTest::gtest_main
)

add_executable(verify_stage_tests verify/VerifyStageTest.cpp)

target_include_directories(verify_stage_tests SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
)
target_include_directories(verify_stage_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/verify/include
  ${CMAKE_SOURCE_DIR}/src/verify
  ${CMAKE_SOURCE_DIR}/src/transform/include
  ${CMAKE_SOURCE_DIR}/src/transform
  ${CMAKE_SOURCE_DIR}/src/filter/include
  ${CMAKE_SOURCE_DIR}/src/common/include
)

target_link_libraries(verify_stage_tests PRIVATE
  verify_lib
  transform_lib
  filter_lib
  ${LLVM_LIBS}
  GTest::gtest_main
)

# gtest_discover_tests registers each TEST() as a separate CTest case,
# so `ctest` output shows individual pass/fail rather than just "filter_tests passed"
include(GoogleTest)
gtest_discover_tests(filter_tests)
gtest_discover_tests(transform_tests)
gtest_discover_tests(transform_stage_tests)
gtest_discover_tests(verify_stage_tests)
