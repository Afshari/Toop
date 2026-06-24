cmake_minimum_required(VERSION 3.20)
project(Toop_Solution)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Shared helpers: CUDA arch targets, define functions
# ---------------------------------------------------------------------------
include(cmake/targets.cmake)

# ---------------------------------------------------------------------------
# Output directory
# Mirrors the SignalForge convention: x64/Release or x64/Debug.
# Works for both Visual Studio (multi-config) and Linux (single-config).
# ---------------------------------------------------------------------------
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/x64/$<CONFIG>)
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
    endif()
    set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/x64/${CMAKE_BUILD_TYPE})
endif()

message(STATUS "Output directory  : ${OUTPUT_DIR}")
message(STATUS "Build type        : ${CMAKE_BUILD_TYPE}")
message(STATUS "CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")

# ---------------------------------------------------------------------------
# Optional build flags
# ---------------------------------------------------------------------------
option(TOOP_HEADLESS "Build without OpenGL (headless/server mode)" OFF)
option(TOOP_DEBUG    "Build with DebugUI and DebugRenderer"        OFF)

if(TOOP_HEADLESS)
    message(STATUS "TOOP_HEADLESS     : ON  - OpenGL stack disabled")
else()
    message(STATUS "TOOP_HEADLESS     : OFF - OpenGL stack enabled")
endif()

if(TOOP_DEBUG)
    message(STATUS "TOOP_DEBUG        : ON  - DebugUI enabled")
endif()

# ---------------------------------------------------------------------------
# Boost (PropertyTree is header-only; we just need the include path)
# Windows: pass -DBOOST_ROOT=C:/vcpkg/installed/x64-windows
# Linux:   installed at /usr/local via source build
# ---------------------------------------------------------------------------
set(Boost_NO_WARN_NEW_VERSIONS 1)
find_package(Boost REQUIRED)
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

# ---------------------------------------------------------------------------
# Sub-projects
# ---------------------------------------------------------------------------
add_subdirectory(Toop_CPU)
add_subdirectory(Toop_GPU)
add_subdirectory(Toop_Bench)

# Interactive exe only makes sense when not headless
if(NOT TOOP_HEADLESS)
    add_subdirectory(Toop)
endif()

add_subdirectory(Toop_Tests)

# ---------------------------------------------------------------------------
# Build order
# ---------------------------------------------------------------------------
add_dependencies(Toop_GPU   Toop_CPU)
add_dependencies(Toop_Bench Toop_CPU Toop_GPU)

if(NOT TOOP_HEADLESS)
    add_dependencies(Toop Toop_CPU Toop_GPU)
endif()

add_dependencies(Toop_Tests Toop_CPU Toop_GPU)

# ---------------------------------------------------------------------------
# Copy config.json next to each output binary
# ---------------------------------------------------------------------------
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    foreach(CONFIG Debug Release)
        file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/x64/${CONFIG})
        configure_file(
            ${CMAKE_SOURCE_DIR}/config.json
            ${CMAKE_SOURCE_DIR}/x64/${CONFIG}/config.json
            COPYONLY
        )
    endforeach()
else()
    file(MAKE_DIRECTORY ${OUTPUT_DIR})
    configure_file(
        ${CMAKE_SOURCE_DIR}/config.json
        ${OUTPUT_DIR}/config.json
        COPYONLY
    )
endif()