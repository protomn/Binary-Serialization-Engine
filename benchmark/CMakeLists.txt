include(FetchContent)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.5
    )

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

file(GLOB BENCHMARK_SOURCES
     CONFIGURE_DEPENDS
     *.cpp)

foreach(benchmark_source ${BENCHMARK_SOURCES})

    get_filename_component(
        benchmark_name
        ${benchmark_source}
        NAME_WE
    )

    add_executable(
        ${benchmark_name}
        ${benchmark_source}
    )

    target_link_libraries(
        ${benchmark_name}
        PRIVATE
            bin_serializer
            benchmark::benchmark
    )

    target_compile_features(
        ${benchmark_name}
        PRIVATE
            cxx_std_23
    )

    target_compile_options(
        ${benchmark_name}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endforeach()