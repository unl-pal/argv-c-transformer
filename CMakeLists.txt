cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
  argv-argc-transformer
  VERSION 0.0.0
  LANGUAGES CXX
)

find_package(LLVM REQUIRED)

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

# filter_lib lets the test binary link the filter pipeline without pulling in main()
add_library(filter_lib STATIC ${SOURCE_FILTER})
target_include_directories(filter_lib SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_FILTER})
target_link_libraries(filter_lib PRIVATE ${LLVM_LIBS})

add_executable(filter src/filter/main.cpp)
add_executable(transform src/transform/main.cpp ${SOURCE_TRANSFORM})
add_executable(full src/full/main.cpp ${SOURCE_TRANSFORM})

target_include_directories(filter SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_FILTER})
target_include_directories(transform SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_TRANSFORM})
target_include_directories(full SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${HEADERS_TRANSFORM} ${HEADERS_FILTER})

target_link_libraries(filter PRIVATE filter_lib ${LLVM_LIBS})
target_link_libraries(transform PRIVATE ${LLVM_LIBS})
target_link_libraries(full PRIVATE filter_lib ${LLVM_LIBS})

if(UNIX)
  set_property(TARGET filter PROPERTY LINKER_TYPE LLD)
  set_property(TARGET transform PROPERTY LINKER_TYPE LLD)
  set_property(TARGET full PROPERTY LINKER_TYPE LLD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused -fcolor-diagnostics")
endif()

enable_testing()
add_subdirectory(tests)

install(TARGETS filter transform full)
