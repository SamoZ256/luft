cmake_minimum_required(VERSION 3.15...3.31)

option(BUILD_TESTS "Build tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(luft VERSION 1.0 LANGUAGES C CXX)

find_package(LLVM REQUIRED CONFIG)
llvm_map_components_to_libnames(LLVM_LIBS support core irreader passes)

add_library(luft SHARED
    src/air_operations.cpp
    src/air_signature.cpp
    src/air_type.cpp
    src/metallib_writer.cpp
)

target_include_directories(luft PRIVATE include ${LLVM_INCLUDE_DIRS})
#add_definitions(${LLVM_DEFINITIONS})
target_link_libraries(luft PRIVATE ${LLVM_LIBS})

if (BUILD_TESTS)
	enable_testing()
    add_subdirectory(tests)
endif()
