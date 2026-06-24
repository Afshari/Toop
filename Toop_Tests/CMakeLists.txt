cmake_minimum_required(VERSION 3.20)
project(Toop_Tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CUDAToolkit REQUIRED)

set(Boost_NO_WARN_NEW_VERSIONS 1)
find_package(Boost REQUIRED)

# ---------------------------------------------------------------------------
# GoogleTest
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(TEST_SOURCES
    pch.cpp
    CLParserTests.cpp
    CameraTests.cpp
    ConfigTests.cpp
    FibonacciSphereTests.cpp
    ShaderProgramTests.cpp
    SpherePhysicsTests.cpp
    HairSimulatorTests.cpp
)

add_executable(Toop_Tests ${TEST_SOURCES})

# ---------------------------------------------------------------------------
# Includes
# ---------------------------------------------------------------------------
target_include_directories(Toop_Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Toop_CPU/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Toop_GPU/inc
    ${Boost_INCLUDE_DIRS}
)

# ---------------------------------------------------------------------------
# Link deps
# ---------------------------------------------------------------------------
target_link_libraries(Toop_Tests PRIVATE
    ${OUTPUT_DIR}/libToop_GPU.a
    ${OUTPUT_DIR}/libToop_CPU.a
    CUDA::cudart_static
    CUDA::curand
    gtest
    gtest_main
)

# ---------------------------------------------------------------------------
# Copy binary and config.json to output
# ---------------------------------------------------------------------------
add_custom_command(TARGET Toop_Tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:Toop_Tests>
        ${OUTPUT_DIR}/Toop_Tests
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/config.json
        ${OUTPUT_DIR}/config.json
)