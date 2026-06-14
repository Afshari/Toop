cmake_minimum_required(VERSION 3.20)
project(Toop)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CUDAToolkit REQUIRED)

set(Boost_NO_WARN_NEW_VERSIONS 1)
find_package(Boost REQUIRED)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SOURCES
    main.cpp
    src/AppRunner.cpp
    src/InputHandler.cpp
)

# DebugUI is only needed when TOOP_DEBUG is set.
# We include it unconditionally here so the target always compiles;
# the #ifdef guards in the source make it a no-op when TOOP_DEBUG is off.
list(APPEND SOURCES src/DebugUI.cpp)

add_executable(Toop ${SOURCES})

# ---------------------------------------------------------------------------
# Include dirs
# ---------------------------------------------------------------------------
target_include_directories(Toop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Toop_CPU/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Toop_GPU/inc
    ${Boost_INCLUDE_DIRS}
)

# ---------------------------------------------------------------------------
# Link deps
# ---------------------------------------------------------------------------
target_link_libraries(Toop PRIVATE
    ${OUTPUT_DIR}/libToop_CPU.a
    ${OUTPUT_DIR}/libToop_GPU.a
    CUDA::cudart_static
    CUDA::curand
)

# ---------------------------------------------------------------------------
# Optional: enable debug UI
# Pass -DTOOP_DEBUG=ON at configure time to include DebugUI/DebugRenderer.
# ---------------------------------------------------------------------------
if(TOOP_DEBUG)
    toop_apply_debug_defines(Toop)
endif()

# ---------------------------------------------------------------------------
# Copy to output
# ---------------------------------------------------------------------------
add_custom_command(TARGET Toop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:Toop>
        ${OUTPUT_DIR}/Toop
)