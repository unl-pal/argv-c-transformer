cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin")
# set(CMAKE_C_STANDARD 99)
# set(CMAKE_C_STANDARD_REQUIRED ON)


project(
  argv-argc-transformer
  VERSION 0.0.0
  LANGUAGES CXX C
)

find_package(Clang REQUIRED)
find_package(LLVM REQUIRED)
# find_package(libgit2 REQUIRED)

set(HEADERS_FILTER
  src/filter/include
)

set(HEADERS_TRANSFORM
  src/transform/include
)

set(HEADERS_EXAMPLE
  src/example/include
)

set(SOURCE_FILTER
  src/filter/CountingVisitor.cpp
  src/filter/Filterer.cpp
  src/filter/Remove.cpp
)

set(SOURCE_TRANSFORM
  src/transform/AddVerifiersConsumer.cpp
  src/transform/ArgsFrontendActionFactory.cpp
  src/transform/AddVerifiersVisitor.cpp
  src/transform/GenerateCodeConsumer.cpp
  src/transform/TransformAction.cpp
  src/transform/GenerateIncludeConsumer.cpp
  src/transform/GenerateIncludeHandler.cpp
  src/transform/RegenCode.cpp
  src/transform/ReplaceDeadCallsVisitor.cpp
  src/transform/ReplaceDeadCallsConsumer.cpp
  src/transform/Transformer.cpp
)

set(SOURCE_EXAMPLE
  # src/example/Example.cpp
  src/example/Action.cpp
  src/example/ConsumerMatcher.cpp
  src/example/ConsumerVisitor.cpp
  src/example/Handler.cpp
  src/example/Visitor.cpp
)

# FetchContent_Declare(
#   <packageName>
#   GIT_REPOSITORY <github_user>/<github_repo>
#   GIT_TAG <git_tag>
# )

include(FetchContent)

# FetchContent_Declare(
#   fmt
#   GIT_REPOSITORY https://github.com/fmtlib/fmt.git
#   GIT_TAG 11.1.3
# )

# FetchContent_GetProperties(fmt)
# if(NOT fmt_POPULATED)
#   FetchContent_MakeAvailable(fmt)
# endif()

# FetchContent_Declare(
#   rapidcsv
#   GIT_REPOSITORY d99kris/rapidcsv
#   GIT_TAG v8.84
# )

include_directories(${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} /usr/local )

add_executable(filter src/filter/main.cpp ${SOURCE_FILTER})
add_executable(transform src/transform/main.cpp ${SOURCE_TRANSFORM})
add_executable(full src/full/main.cpp ${SOURCE_FILTER} ${SOURCE_TRANSFORM})
add_executable(example src/example/Main.cpp ${SOURCE_EXAMPLE})

target_include_directories(filter SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} /usr/local /usr/lib64 ${HEADERS_FILTER})
target_include_directories(transform SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} ${HEADERS_TRANSFORM})
target_include_directories(full SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} ${HEADERS_TRANSFORM} ${HEADERS_DOWNLOAD} ${HEADERS_FILTER})
target_include_directories(example SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${Clang_INCLUDE_DIRS} ${LLVM_INC} ${CLANG_INC} ${HEADERS_EXAMPLE})

target_link_libraries(filter PRIVATE LLVM LLVMSupport clang-cpp clang)
target_link_libraries(transform PRIVATE LLVMSupport clang-cpp clang LLVM)
target_link_libraries(full PRIVATE LLVMSupport clang-cpp clang LLVM)
target_link_libraries(example PRIVATE LLVMSupport clang-cpp clang LLVM)

if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W4")

elseif(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused")

elseif(UNIX)
  set_property(TARGET filter PROPERTY LINKER_TYPE LLD)
  set_property(TARGET transform PROPERTY LINKER_TYPE LLD)
  set_property(TARGET full PROPERTY LINKER_TYPE LLD)
  set_property(TARGET example PROPERTY LINKER_TYPE LLD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused")

endif()

install(TARGETS filter transform full)
