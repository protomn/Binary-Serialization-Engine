include(FetchContent)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.5
    )

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

add_executable(benchmark_serializer
        benchmark_trivial.cpp)

target_link_libraries(benchmark_serializer
    PRIVATE
        bin_serializer
        benchmark::benchmark)

target_compile_features(benchmark_serializer
    PRIVATE
        cxx_std_20)