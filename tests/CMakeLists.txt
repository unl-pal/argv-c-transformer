include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
  URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
)
# Prevent GTest from overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(filter_tests filter/CountingVisitorTest.cpp)

target_include_directories(filter_tests SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
)
target_include_directories(filter_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/filter/include
  ${CMAKE_SOURCE_DIR}/src/filter
)

target_link_libraries(filter_tests PRIVATE
  filter_lib
  ${LLVM_LIBS}
  GTest::gtest_main
)

# gtest_discover_tests registers each TEST() as a separate CTest case,
# so `ctest` output shows individual pass/fail rather than just "filter_tests passed"
include(GoogleTest)
gtest_discover_tests(filter_tests)
