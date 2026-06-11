#include "InputHandler.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Toop {

    // --------------------------------------------------------------------------------
    void InputHandler::Init(
        Window* window,
        Camera* camera,
        SpherePhysics* sphere,
        const Config* config)
    {
        m_window = window;
        m_camera = camera;
        m_sphere = sphere;
        m_config = config;
    }

    // --------------------------------------------------------------------------------
    void InputHandler::RegisterCallbacks()
    {
        // key callback
        m_window->SetKeyCallback([this](int key, int action)
            {
                OnKey(key, action);
            });

        // mouse callback
        m_window->SetMouseCallback([this](const MouseState& mouse)
            {
                OnMouse(mouse,
                    m_camera->GetViewMatrix(),
                    m_camera->GetProjectionMatrix(
                        (float)m_window->GetWidth() / (float)m_window->GetHeight()));
            });

        // scroll callback
        m_window->SetScrollCallback([this](float delta)
            {
                m_camera->HandleWheel(delta);
            });
    }

    // --------------------------------------------------------------------------------
    void InputHandler::UpdateMouseWorld(
        const glm::mat4& view,
        const glm::mat4& proj)
    {
        const auto& mouse = m_window->GetMouseState();

        Ray ray = RayUtils::ScreenToRay(
            mouse.x, mouse.y,
            m_window->GetWidth(), m_window->GetHeight(),
            view, proj);

        glm::vec3 sphere_center(
            m_sphere->GetPosX(),
            m_sphere->GetPosY(),
            m_sphere->GetPosZ());

        RayUtils::RayCameraPlaneIntersect(
            ray, sphere_center,
            m_camera->GetForward(), m_mouse_world);
    }

    // --------------------------------------------------------------------------------
    void InputHandler::OnKey(int key, int action)
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            glm::vec3 sphere_pos(
                m_sphere->GetPosX(),
                m_sphere->GetPosY(),
                m_sphere->GetPosZ());

            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(m_window->GetGLFWWindow(), true);
                break;
            case GLFW_KEY_W: m_camera->HandleKeyW(); break;
            case GLFW_KEY_S: m_camera->HandleKeyS(); break;
            case GLFW_KEY_A: m_camera->HandleKeyA(); break;
            case GLFW_KEY_D: m_camera->HandleKeyD(); break;
            case GLFW_KEY_Q: m_camera->HandleKeyQ(); break;
            case GLFW_KEY_E: m_camera->HandleKeyE(); break;
            case GLFW_KEY_1: m_camera->SetPreset(1, sphere_pos); break;
            case GLFW_KEY_2: m_camera->SetPreset(2, sphere_pos); break;
            case GLFW_KEY_3: m_camera->SetPreset(3, sphere_pos); break;
            case GLFW_KEY_4: m_camera->SetPreset(4, sphere_pos); break;
            }
        }
    }

    // --------------------------------------------------------------------------------
    void InputHandler::OnMouse(
        const MouseState& mouse,
        const glm::mat4& view,
        const glm::mat4& proj)
    {
        Ray ray = RayUtils::ScreenToRay(
            mouse.x, mouse.y,
            m_window->GetWidth(), m_window->GetHeight(),
            view, proj);

        glm::vec3 sphere_pos(
            m_sphere->GetPosX(),
            m_sphere->GetPosY(),
            m_sphere->GetPosZ());

        bool just_pressed = mouse.left && !m_prev_left;
        bool just_released = !mouse.left && m_prev_left;
        m_prev_left = mouse.left;

        if (just_pressed)
        {
            if (RayUtils::RaySphereIntersect(ray, sphere_pos,
                m_config->sim.sphere_radius))
            {
                m_is_dragging = true;
                m_drag_plane_center = sphere_pos;
                m_sphere->StartDrag(sphere_pos.x, sphere_pos.y, sphere_pos.z);
            }
        }

        if (just_released && m_is_dragging)
        {
            m_is_dragging = false;
            m_sphere->EndDrag();
        }

        if (mouse.left && !m_is_dragging)
            m_camera->HandleMouseOrbit(mouse.dx, mouse.dy, sphere_pos);

        if (mouse.left && m_is_dragging)
        {
            glm::vec3 hit;
            if (RayUtils::RayCameraPlaneIntersect(ray,
                m_drag_plane_center,
                m_camera->GetForward(), hit))
            {
                m_drag_plane_center = hit;
                m_sphere->UpdateDrag(hit.x, hit.y, hit.z);
            }
        }

        if (mouse.right)
            m_camera->HandleMouseTranslate(mouse.dx, mouse.dy);
    }

} // namespace Toop