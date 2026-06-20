#pragma once
#ifndef TOOP_HEADLESS
#include "Config.h"
#include "Camera.h"
#include "SpherePhysics.h"
#include "RayUtils.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "cpu/Window.h"

#ifdef TOOP_DEBUG
#include "DebugManager.h"
#include "cpu/Renderer.h"
#endif

namespace Toop {

    class InputHandler
    {
    public:
        void Init(
            Window* window,
            Camera* camera,
            SpherePhysics* sphere,
            const Config* config
#ifdef TOOP_DEBUG
            , DebugManager* debug_manager = nullptr
            , Renderer* renderer = nullptr
#endif
        );

        void RegisterCallbacks();

        // getters for AppRunner
        bool             IsDragging()        const { return m_is_dragging; }
        const glm::vec3& GetMouseWorld()     const { return m_mouse_world; }

        // called every frame from main loop
        void UpdateMouseWorld(const glm::mat4& view, const glm::mat4& proj);
        const glm::vec3& GetDragPlaneCenter() const { return m_drag_plane_center; }

    private:
        void OnKey(int key, int action);
        void OnMouse(const MouseState& mouse,
            const glm::mat4& view,
            const glm::mat4& proj);
        void OnScroll(float delta);

        Window* m_window = nullptr;
        Camera* m_camera = nullptr;
        SpherePhysics* m_sphere = nullptr;
        const Config* m_config = nullptr;

        bool      m_is_dragging = false;
        bool      m_prev_left = false;
        glm::vec3 m_drag_plane_center = {};
        glm::vec3 m_mouse_world = {};

        Ray       m_last_ray = {};

#ifdef TOOP_DEBUG
        DebugManager* m_debug_manager = nullptr;
        Renderer* m_renderer = nullptr;
#endif
    };

} // namespace Toop
#endif // TOOP_HEADLESS
