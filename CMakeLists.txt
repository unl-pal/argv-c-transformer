cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
  argv-argc-transformer
  VERSION 0.0.0
  LANGUAGES CXX C
)

find_package(Clang REQUIRED)
find_package(LLVM REQUIRED)

# NOTE: If adding the downloader to the build system is desirable
# find_package(Python3 REQUIRED COMPONENTS Interpreter)
# add_custom_target(
#   download
#   COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src/download/Downloader.py
#   COMMENT "Running the Downloader"
# )
# NOTE: Immediatly executes instead of on user command

# Can be used to enforce downloader to run before filter?
# add_dependencies(filter download)


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
  src/filter/AddVerifiersConsumerFilter.cpp
  src/filter/AddVerifiersVisitorFilter.cpp
  src/filter/CountingConsumer.cpp
  src/filter/CountingVisitor.cpp
  src/filter/FilterAction.cpp
  src/filter/Filterer.cpp
  src/filter/FilterFunctionsConsumer.cpp
  src/filter/FrontendFactoryWithArgs.cpp
  src/filter/RemoveConsumer.cpp
  src/filter/RemoveVisitor.cpp
)

set(SOURCE_TRANSFORM
  src/transform/AddVerifiersConsumer.cpp
  src/transform/AddVerifiersVisitor.cpp
  src/transform/ArgsFrontendActionFactory.cpp
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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused -fcolor-diagnostics")

endif()

install(TARGETS filter transform full)
