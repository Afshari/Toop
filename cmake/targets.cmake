# cmake/targets.cmake
# Shared CUDA architecture and compile-definition helpers.
# Included by the root CMakeLists.txt before add_subdirectory calls.

# ---------------------------------------------------------------------------
# CUDA architectures
# ---------------------------------------------------------------------------
# Default: all three target platforms.
#   86 - RTX 3060 Laptop (local dev)
#   75 - Tesla T4       (AWS)
#   53 - Jetson Nano    (edge)
#
# Override at configure time:
#   cmake .. -DCMAKE_CUDA_ARCHITECTURES=75
# ---------------------------------------------------------------------------
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 86 75 53)
endif()

message(STATUS "CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")

# ---------------------------------------------------------------------------
# Build mode helpers
# ---------------------------------------------------------------------------
# TOOP_HEADLESS - strips OpenGL, GLFW, GLAD, ImGui, interop
# TOOP_DEBUG    - adds DebugUI, DebugRenderer (never set in headless builds)
#
# Usage in subdirectory CMakeLists.txt:
#   toop_apply_headless_defines(<target>)
#   toop_apply_debug_defines(<target>)
# ---------------------------------------------------------------------------
function(toop_apply_headless_defines target)
    target_compile_definitions(${target} PRIVATE TOOP_HEADLESS)
endfunction()

function(toop_apply_debug_defines target)
    target_compile_definitions(${target} PRIVATE TOOP_DEBUG)
endfunction()