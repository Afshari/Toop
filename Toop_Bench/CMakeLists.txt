cmake_minimum_required(VERSION 3.20)
project(Toop_Bench)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CUDAToolkit REQUIRED)

set(Boost_NO_WARN_NEW_VERSIONS 1)
find_package(Boost REQUIRED)

# ---------------------------------------------------------------------------
# Toop_Bench reuses Toop's main.cpp and AppRunner.cpp directly.
# They are compiled again here with TOOP_HEADLESS defined, so all
# OpenGL/GLFW/GLAD/ImGui paths are compiled out via #ifdef.
# ---------------------------------------------------------------------------
set(SOURCES
    ${CMAKE_SOURCE_DIR}/Toop/main.cpp
    ${CMAKE_SOURCE_DIR}/Toop/src/AppRunner.cpp
)

add_executable(Toop_Bench ${SOURCES})

# TOOP_HEADLESS must be set before any headers are parsed
target_compile_definitions(Toop_Bench PRIVATE TOOP_HEADLESS)

# ---------------------------------------------------------------------------
# Include dirs - same as Toop exe, minus OpenGL headers
# ---------------------------------------------------------------------------
target_include_directories(Toop_Bench PRIVATE
    ${CMAKE_SOURCE_DIR}/Toop/inc
    ${CMAKE_SOURCE_DIR}/Toop_CPU/inc
    ${CMAKE_SOURCE_DIR}/Toop_GPU/inc
    ${Boost_INCLUDE_DIRS}
)

# ---------------------------------------------------------------------------
# Link deps - no OpenGL, no GLFW, no GLAD
# ---------------------------------------------------------------------------
target_link_libraries(Toop_Bench PRIVATE
    ${OUTPUT_DIR}/libToop_CPU.a
    ${OUTPUT_DIR}/libToop_GPU.a
    CUDA::cudart_static
    CUDA::curand
)

# ---------------------------------------------------------------------------
# Copy to output
# ---------------------------------------------------------------------------
add_custom_command(TARGET Toop_Bench POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:Toop_Bench>
        ${OUTPUT_DIR}/Toop_Bench
)