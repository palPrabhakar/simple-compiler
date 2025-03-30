cmake_minimum_required(VERSION 3.10)

project(SimpleCompiler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    sjp
    GIT_REPOSITORY git@github.com:palPrabhakar/simple-json-parser.git
    GIT_TAG main
)

FetchContent_MakeAvailable(sjp)

# Source Files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable Target
add_executable(sc ${SOURCES})
target_include_directories(sc PRIVATE include/)
target_link_libraries(sc sjp)

# Enable Testing only in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Testing
    enable_testing()

    # Google Test Integration
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})

    # Create a separate executable for tests
    add_executable(test_sc test/my_tests.cpp ${GTEST_SOURCE})
    target_link_libraries(test_sc GTest::gtest GTest::gtest_main)

    # Add a test that runs the test executable
    add_test(NAME unit_test COMMAND test_sc)
endif()
