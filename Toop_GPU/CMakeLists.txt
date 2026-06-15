cmake_minimum_required(VERSION 3.20)
project(Toop_GPU CUDA CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

find_package(CUDAToolkit REQUIRED)

# ---------------------------------------------------------------------------
# Sources - always compiled (pure CUDA, no OpenGL dependency)
# ---------------------------------------------------------------------------
set(GPU_SOURCES
    src/gpu/HairSim.cu
    src/gpu/HairSimulator.cu
)

# ---------------------------------------------------------------------------
# Sources - only when building with OpenGL (interactive mode)
# ---------------------------------------------------------------------------
if(NOT TOOP_HEADLESS)
    find_package(OpenGL REQUIRED)
    find_package(glfw3  REQUIRED)

    # GLAD - shipped as source in many setups; adjust path if needed
    find_package(glad   QUIET)

    list(APPEND GPU_SOURCES
        src/cpu/Renderer.cpp
        src/cpu/Window.cpp
        src/cpu/ShaderProgram.cpp
        src/cpu/DebugRenderer.cpp
    )
endif()

add_library(Toop_GPU STATIC ${GPU_SOURCES})

set_target_properties(Toop_GPU PROPERTIES
    CUDA_SEPARABLE_COMPILATION OFF
)

# ---------------------------------------------------------------------------
# Include dirs
# ---------------------------------------------------------------------------
target_include_directories(Toop_GPU PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Toop_CPU/inc
)

# ---------------------------------------------------------------------------
# Link deps
# ---------------------------------------------------------------------------
target_link_libraries(Toop_GPU PUBLIC
    CUDA::cudart_static
    CUDA::curand
)

if(NOT TOOP_HEADLESS)
    target_link_libraries(Toop_GPU PUBLIC
        OpenGL::GL
        glfw
    )
    if(glad_FOUND)
        target_link_libraries(Toop_GPU PUBLIC glad::glad)
    endif()
endif()

# ---------------------------------------------------------------------------
# Defines forwarded to consumers
# ---------------------------------------------------------------------------
if(TOOP_HEADLESS)
    target_compile_definitions(Toop_GPU PUBLIC TOOP_HEADLESS)
endif()

# ---------------------------------------------------------------------------
# Copy to output
# ---------------------------------------------------------------------------
add_custom_command(TARGET Toop_GPU POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:Toop_GPU>
        ${OUTPUT_DIR}/libToop_GPU.a
)