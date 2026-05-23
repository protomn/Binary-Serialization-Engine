include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.15.0
)

FetchContent_MakeAvailable(Catch2)

include(Catch)
include(CTest)

file(GLOB TEST_SOURCES
     CONFIGURE_DEPENDS
     *.cpp)

foreach(test_source ${TEST_SOURCES})

        get_filename_component(
                test_name
                ${test_source}
                NAME_WE
        )

        add_executable(
                ${test_name}
                ${test_source}
        )

        target_link_libraries(
                ${test_name}
                PRIVATE
                        bin_serializer
                        Catch2::Catch2WithMain
        )

        catch_discover_tests(${test_name})
endforeach()

