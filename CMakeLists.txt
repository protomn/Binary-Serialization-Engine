cmake_minimum_required(VERSION 3.20)

project(bin_serializer
        VERSION 0.1.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(bin_serializer
        src/buffer.cpp
        src/serialize.cpp)

target_include_directories(bin_serializer
        PUBLIC
        include
)

target_compile_features(bin_serializer PUBLIC cxx_std_20)

target_compile_options(bin_serializer PRIVATE
        -Wall
        -Wextra
        -Wpedantic)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(bin_serializer PRIVATE
            -fsanitize=address,undefined
            )
    target_link_options(bin_serializer PRIVATE
            -fsanitize=address,undefined
            )
endif()

add_subdirectory(tests)
add_subdirectory(benchmark)