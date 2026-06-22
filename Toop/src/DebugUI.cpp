#include "DebugUI.h"

#ifdef TOOP_DEBUG

#include <iostream>

namespace Toop {

    // --------------------------------------------------------------------------------
    void DebugUI::Init(GLFWwindow* window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 430");

        m_initialized = true;
        std::cout << "[INFO] DebugUI initialized." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void DebugUI::Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
        std::cout << "[INFO] DebugUI shutdown." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void DebugUI::BeginFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    // --------------------------------------------------------------------------------
    void DebugUI::Render(DebugUIState& state, DebugContext& debug_context)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Always);
        ImGui::Begin("Toop Debug", nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove);

        // performance
        if (ImGui::CollapsingHeader("Performance"))
        {
            ImGui::Text("FPS:      %.1f", state.fps);
            ImGui::Text("Frame:    %.2f ms", state.frame_ms);
            ImGui::Text("Sphere speed: %.3f", state.sphere_speed);
        }

        // simulation
        if (ImGui::CollapsingHeader("Simulation"))
        {
            ImGui::Checkbox("Freeze sim", &state.freeze_sim);
            ImGui::Checkbox("Show hair", &state.show_hair);
            ImGui::SliderInt("Substeps", &state.num_substeps, 1, 60);
            ImGui::SliderFloat("Compliance", &state.compliance, 0.0f, 0.01f, "%.6f");
            ImGui::SliderFloat("Damping", &state.damping, 0.9f, 1.0f, "%.4f");
            ImGui::SliderFloat("Gravity", &state.gravity, -5.0f, 0.0f, "%.2f");
        }

        // wind
        if (ImGui::CollapsingHeader("Wind"))
        {
            ImGui::SliderFloat("Strength", &state.wind_strength, 0.0f, 1.0f, "%.3f");
            ImGui::SliderFloat("Frequency", &state.wind_frequency, 0.0f, 2.0f, "%.3f");
        }

        // debug overlays
        ImGui::SetNextItemOpen(debug_context.show_debug_panel, ImGuiCond_Always);
        if (ImGui::CollapsingHeader("Debug Overlays"))
        {
            ImGui::Checkbox("Local Axes (A)", &debug_context.show_local_axes);
            ImGui::Checkbox("Orientation Cubes (Y)", &debug_context.show_orientation_cubes);
            ImGui::Checkbox("Camera Ray (X)", &debug_context.show_camera_ray);
            ImGui::Checkbox("Eye Planes", &debug_context.show_eye_planes);
            ImGui::Checkbox("Drag Plane (Z)", &debug_context.show_drag_plane);
            ImGui::Checkbox("Freeze Sphere (Q)", &debug_context.frozen);
            ImGui::Checkbox("Hide UI (F)", &debug_context.show_imgui);
            ImGui::Text("Snapshots: %d (R/C)", (int)debug_context.snapshots.size());
        }
        // sync back in case user clicked the header manually
        debug_context.show_debug_panel = ImGui::IsItemToggledOpen()
            ? !debug_context.show_debug_panel
            : debug_context.show_debug_panel;

        ImGui::End();
    }

    // --------------------------------------------------------------------------------
    void DebugUI::EndFrame()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

} // namespace Toop

#endif // TOOP_DEBUG