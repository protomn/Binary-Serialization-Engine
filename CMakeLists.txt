cmake_minimum_required(VERSION 3.30)

project(bin_serializer
        VERSION 0.1.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_BENCHMARKS "Build the Google Benchmark suite" ON)
option(ENABLE_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(ENABLE_WERROR "Treat compiler warnings as errors" OFF)

enable_testing()


if(ENABLE_SANITIZERS OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            )
    add_link_options(
            -fsanitize=address,undefined
            )
endif()

add_library(bin_serializer
        src/buffer.cpp
)

target_include_directories(bin_serializer
        PUBLIC
        include
)

target_compile_features(bin_serializer PUBLIC cxx_std_23)

target_compile_options(bin_serializer PRIVATE
        -Wall
        -Wextra
        -Wpedantic)

if(ENABLE_WERROR)
    target_compile_options(bin_serializer PRIVATE -Werror)
endif()

add_subdirectory(tests)

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()
