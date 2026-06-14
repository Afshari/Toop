cmake_minimum_required(VERSION 3.20)
project(Toop_CPU)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost PropertyTree is header-only; we still use find_package for
# portability across Linux (source install) and Windows (vcpkg).
set(Boost_NO_WARN_NEW_VERSIONS 1)
find_package(Boost REQUIRED)

set(CPU_SOURCES
    src/CLParser.cpp
    src/Camera.cpp
    src/Config.cpp
    src/FibonacciSphere.cpp
    src/RayUtils.cpp
    src/SpherePhysics.cpp
)

add_library(Toop_CPU STATIC ${CPU_SOURCES})

target_include_directories(Toop_CPU PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${Boost_INCLUDE_DIRS}
)

# No link deps - Boost.PropertyTree is header-only,
# everything else is stdlib.

add_custom_command(TARGET Toop_CPU POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:Toop_CPU>
        ${OUTPUT_DIR}/libToop_CPU.a
)