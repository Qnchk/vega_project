cmake_minimum_required(VERSION 3.20)
project(lwti LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LWTI_BUILD_TESTS "Build unit tests" ON)

add_library(lwti_lib
    src/indicator.cpp
    src/csv_reader.cpp
)
target_include_directories(lwti_lib PUBLIC include)
target_compile_features(lwti_lib PUBLIC cxx_std_20)
target_compile_options(lwti_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(lwti src/main.cpp)
target_link_libraries(lwti PRIVATE lwti_lib)

if(LWTI_BUILD_TESTS)
    enable_testing()
    add_library(Catch2 INTERFACE)
    add_library(Catch2::Catch2 ALIAS Catch2)
    target_include_directories(Catch2 INTERFACE tests)

    add_executable(lwti_tests tests/test_indicator.cpp tests/catch_amalgamated.cpp)
    target_link_libraries(lwti_tests PRIVATE lwti_lib Catch2::Catch2)
    add_test(NAME lwti_tests COMMAND lwti_tests)
endif()
