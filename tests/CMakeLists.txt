include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.15.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(test_trivial
        test_trivial.cpp)

target_link_libraries(test_trivial
        PRIVATE
        bin_serializer
        Catch2::Catch2WithMain)

include(CTest)
include(Catch)

catch_discover_tests(test_trivial)