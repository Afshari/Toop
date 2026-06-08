#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <functional>
#include <string>

namespace Toop {

    struct MouseState
    {
        float x = 0.0f;
        float y = 0.0f;
        float dx = 0.0f;
        float dy = 0.0f;
        bool  left = false;
        bool  right = false;
        bool  middle = false;
        float scroll = 0.0f;
    };

    class Window
    {
    public:
        // callbacks
        using KeyCallback = std::function<void(int key, int action)>;
        using MouseCallback = std::function<void(const MouseState&)>;
        using ScrollCallback = std::function<void(float delta)>;
        using ResizeCallback = std::function<void(int width, int height)>;

        Window(const std::string& title, int width, int height, bool vsync);
        ~Window();

        // non-copyable
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        void BeginFrame();
        void EndFrame();
        bool ShouldClose() const;

        // callback registration
        void SetKeyCallback(KeyCallback    cb) { m_key_cb = cb; }
        void SetMouseCallback(MouseCallback  cb) { m_mouse_cb = cb; }
        void SetScrollCallback(ScrollCallback cb) { m_scroll_cb = cb; }
        void SetResizeCallback(ResizeCallback cb) { m_resize_cb = cb; }

        int   GetWidth()  const { return m_width; }
        int   GetHeight() const { return m_height; }
        float GetAspect() const { return (float)m_width / (float)m_height; }

        const MouseState& GetMouseState() const { return m_mouse; }

    private:
        // GLFW static callbacks - forward to instance
        static void GlfwKeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);
        static void GlfwMousePosCallback(GLFWwindow* w, double x, double y);
        static void GlfwMouseBtnCallback(GLFWwindow* w, int button, int action, int mods);
        static void GlfwScrollCallback(GLFWwindow* w, double dx, double dy);
        static void GlfwResizeCallback(GLFWwindow* w, int width, int height);

        GLFWwindow* m_window = nullptr;
        int         m_width = 0;
        int         m_height = 0;
        bool        m_vsync = true;
        MouseState  m_mouse;

        KeyCallback    m_key_cb;
        MouseCallback  m_mouse_cb;
        ScrollCallback m_scroll_cb;
        ResizeCallback m_resize_cb;
    };

} // namespace Toop