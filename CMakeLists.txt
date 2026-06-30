cmake_minimum_required(VERSION 3.5)
project(FactorExpressionParser LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fep_core STATIC src/ast.cpp src/factor_evaluator.cpp src/fepCore.cpp src/Lexer.cpp src/Parser.cpp)
target_include_directories(fep_core PUBLIC src)

add_executable(fep src/main.cpp)
target_link_libraries(fep PRIVATE fep_core)

# Tests are optional — build with: cmake -B build -S . -DBUILD_TESTS=ON
option(BUILD_TESTS "Build unit tests using Catch2" ON)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

