cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)


project(
  MyTransform
  VERSION 0.0.0
  LANGUAGES CXX C
)

find_package(Clang REQUIRED)
find_package(LLVM REQUIRED)
# find_package(libgit2 REQUIRED)

set(HEADERS_DOWNLOAD
  src/download/include
)

set(HEADERS_FILTER
  src/filter/include
)
set(HEADERS_TRANSFORM
  src/transform/include
)

set(SOURCE_DOWNLOAD
  src/download/download.cpp
  src/download/gitclone.cpp
  src/download/chooseUrls.cpp
)

set(SOURCE_FILTER
  src/filter/Filter.cpp
  src/filter/Filterer.cpp
  src/filter/Remove.cpp
)

set(SOURCE_TRANSFORM
  src/transform/Transformer.cpp
  src/transform/Transform.cpp
  src/transform/ReGenCode.cpp
  src/transform/srcCodeGenerator.cpp
)

# FetchContent_Declare(
#   <packageName>
#   GIT_REPOSITORY <github_user>/<github_repo>
#   GIT_TAG <git_tag>
# )

include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.1.3
)

FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
  FetchContent_MakeAvailable(fmt)
endif()

# FetchContent_Declare(
#   rapidcsv
#   GIT_REPOSITORY d99kris/rapidcsv
#   GIT_TAG v8.84
# )

include_directories(${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} /usr/local )

add_executable(download src/download/main.cpp ${SOURCE_DOWNLOAD})
add_executable(filter src/filter/main.cpp ${SOURCE_FILTER})
add_executable(transform src/transform/main.cpp ${SOURCE_TRANSFORM})
add_executable(full src/full/main.cpp ${SOURCE_FILTER} ${SOURCE_DOWNLOAD} ${SOURCE_TRANSFORM})

target_include_directories(download SYSTEM PRIVATE ${HEADERS_DOWNLOAD}) # ${libgit2_INC})
target_include_directories(filter SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} /usr/local /usr/lib64 ${HEADERS_FILTER})
target_include_directories(transform SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} ${HEADERS_TRANSFORM})
target_include_directories(full SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} ${HEADERS_TRANSFORM} ${HEADERS_DOWNLOAD} ${HEADERS_FILTER})

target_link_libraries(download PRIVATE fmt) # git2 rapidcsv)
target_link_libraries(filter PRIVATE LLVM LLVMSupport clang-cpp clang)
target_link_libraries(transform PRIVATE LLVMSupport clang-cpp clang LLVM)
target_link_libraries(full PRIVATE fmt LLVMSupport clang-cpp clang LLVM)

set_property(TARGET filter PROPERTY LINKER_TYPE LLD)
set_property(TARGET download PROPERTY LINKER_TYPE LLD)
set_property(TARGET transform PROPERTY LINKER_TYPE LLD)
set_property(TARGET full PROPERTY LINKER_TYPE LLD)

if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W4")

elseif(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused")

elseif(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused")

endif()

#install(TARGETS {name} DESTINATION {abs_loc})
