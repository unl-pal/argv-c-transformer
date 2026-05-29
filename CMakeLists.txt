cmake_minimum_required(VERSION 3.20)

if(APPLE)
  set(LLVM_HOMEBREW_PREFIX /opt/homebrew/opt/llvm@20)
  set(CMAKE_CXX_COMPILER ${LLVM_HOMEBREW_PREFIX}/bin/clang++)
  set(CMAKE_C_COMPILER ${LLVM_HOMEBREW_PREFIX}/bin/clang)
  list(APPEND CMAKE_PREFIX_PATH ${LLVM_HOMEBREW_PREFIX})
endif()

set(CMAKE_CXX_STANDARD 17)
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
  src/transform/AddMainVisitor.cpp
  src/transform/AddVerifiersConsumer.cpp
  src/transform/AddVerifiersVisitor.cpp
  src/transform/AllFunctionsToCallHandler.cpp
  src/transform/ArgsFrontendActionFactory.cpp
  src/transform/IsThereMainConsumer.cpp
  src/transform/IsThereMainHandler.cpp
  src/transform/GenerateCodeConsumer.cpp
  src/transform/GenerateIncludeConsumer.cpp
  src/transform/GenerateIncludeHandler.cpp
  src/transform/RegenCode.cpp
  src/transform/ReplaceDeadCallsVisitor.cpp
  src/transform/ReplaceDeadCallsConsumer.cpp
  src/transform/TransformAction.cpp
  src/transform/Transformer.cpp
)

add_executable(filter src/filter/main.cpp ${SOURCE_FILTER})
add_executable(transform src/transform/main.cpp ${SOURCE_TRANSFORM})
add_executable(full src/full/main.cpp ${SOURCE_FILTER} ${SOURCE_TRANSFORM})

target_include_directories(filter SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_FILTER})
target_include_directories(transform SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_TRANSFORM})
target_include_directories(full SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_TRANSFORM} ${HEADERS_FILTER})

target_link_libraries(filter PRIVATE ${LLVM_LIBS})
target_link_libraries(transform PRIVATE ${LLVM_LIBS})
target_link_libraries(full PRIVATE ${LLVM_LIBS})

if(UNIX)
  set_property(TARGET filter PROPERTY LINKER_TYPE LLD)
  set_property(TARGET transform PROPERTY LINKER_TYPE LLD)
  set_property(TARGET full PROPERTY LINKER_TYPE LLD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused -fcolor-diagnostics")
endif()

install(TARGETS filter transform full)
