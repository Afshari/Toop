#pragma once
#ifndef TOOP_HEADLESS
#ifdef TOOP_DEBUG
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include "Config.h"

namespace Toop {

    struct DebugUIState
    {
        // simulation toggles
        bool freeze_sim = false;
        bool show_hair = true;
        bool show_debug_rays = false;
        bool show_drag_plane = false;
        bool show_velocity = false;
        bool show_light = false;

        // tunable params - mirror config values
        float compliance = 0.0f;
        float damping = 0.0f;
        float wind_strength = 0.0f;
        float wind_frequency = 0.0f;
        float gravity = 0.0f;
        int   num_substeps = 0;

        // read-only display
        float fps = 0.0f;
        float frame_ms = 0.0f;
        float sphere_speed = 0.0f;
    };

    class DebugUI
    {
    public:
        void Init(GLFWwindow* window);
        void Shutdown();

        void BeginFrame();
        void Render(DebugUIState& state);
        void EndFrame();

        bool IsInitialized() const { return m_initialized; }

    private:
        bool m_initialized = false;
    };

} // namespace Toop
#endif // TOOP_DEBUG
#endif // TOOP_HEADLESS