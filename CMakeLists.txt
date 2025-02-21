cmake_minimum_required(VERSION 3.30.7)

project(
  MyTransform
  VERSION 0.0.0
  LANGUAGES CXX C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin")
# set(CMAKE_C_STANDARD 20)
# set(CMAKE_C_STANDARD_REQUIRED ON)

set(HEADERS_DOWNLOAD
  src/download/include
)

set(HEADERS_FILTER
  src/filter/include
)
set(HEADERS_TRANSFORM
  src/transform/include
)

set(HEADERS_EXPERIMENT
  src/transform/include
)

set(SOURCE_DOWNLOAD
  src/download/download.cpp
  src/download/gitclone.cpp
  src/download/chooseUrls.cpp
)

set(SOURCE_FILTER
  src/filter/Filter.cpp
)

set(SOURCE_TRANSFORM
  # src/transform/create_ast.cpp
  src/transform/pretty_print_visitor.cpp
  src/transform/transform.cpp
)

set(SOURCE_EXPERIMENT
  src/transform/experiment.cpp
  src/transform/IfRewrite.cpp
  src/transform/MyRewrite.cpp
)

set(CPM_DOWNLOAD_VERSION v0.40.5) 
set(CPM_DOWNLOAD_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS $(CPM_DOWNLOAD_LOCATION)))
  message(STATUS "Downloading CPM.cmake")
  file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage("gh:fmtlib/fmt#11.1.3")
CPMAddPackage("gh:d99kris/rapidcsv#v8.84")

find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)
find_package(libgit2 REQUIRED)

include_directories(${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} /usr/local include)

add_executable(download src/download/main.cpp ${SOURCE_DOWNLOAD})
add_executable(filter src/filter/main.cpp ${SOURCE_FILTER})
add_executable(transform src/transform/main.cpp ${SOURCE_TRANSFORM})
add_executable(experiment ${SOURCE_EXPERIMENT})

target_include_directories(transform PRIVATE ${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} /usr/local /usr/lib64 include/transform)
target_include_directories(experiment PRIVATE ${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} /usr/local /usr/lib64 ${HEADERS_TRANSFORM})
target_include_directories(download PRIVATE ${HEADERS_DOWNLOAD} ${libgit2_INC})

target_link_libraries(download PRIVATE fmt::fmt git2 rapidcsv)
target_link_libraries(filter PRIVATE LLVM LLVMSupport clang-cpp clang)
target_link_libraries(transform PRIVATE LLVMSupport clang-cpp clang LLVM)
target_link_libraries(experiment PRIVATE fmt::fmt LLVMSupport clang-cpp clang LLVM)

if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W4")

elseif(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused")

elseif(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused")

endif()

#install(TARGETS {name} DESTINATION {abs_loc})
