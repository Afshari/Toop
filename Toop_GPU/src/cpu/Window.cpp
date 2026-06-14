#include "cpu/Window.h"
#include <stdexcept>
#include <iostream>

extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 1;
}

namespace Toop {

    // --------------------------------------------------------------------------------
    Window::Window(
        const std::string& title,
        int                width,
        int                height,
        bool               vsync)
        : m_width(width)
        , m_height(height)
        , m_vsync(vsync)
    {

        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WIN32);

        if (!glfwInit())
            throw std::runtime_error("[ERROR] GLFW init failed.");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!m_window)
        {
            glfwTerminate();
            throw std::runtime_error("[ERROR] GLFW window creation failed.");
        }

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(vsync ? 1 : 0);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            glfwDestroyWindow(m_window);
            glfwTerminate();
            throw std::runtime_error("[ERROR] GLAD init failed.");
        }

        // store pointer to this instance for static callbacks
        glfwSetWindowUserPointer(m_window, this);

        glfwSetKeyCallback(m_window, GlfwKeyCallback);
        glfwSetCursorPosCallback(m_window, GlfwMousePosCallback);
        glfwSetMouseButtonCallback(m_window, GlfwMouseBtnCallback);
        glfwSetScrollCallback(m_window, GlfwScrollCallback);
        glfwSetFramebufferSizeCallback(m_window, GlfwResizeCallback);

        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);

        std::cout << "[INFO] Window created: " << width << "x" << height << std::endl;
        std::cout << "[INFO] OpenGL version: "
            << glGetString(GL_VERSION) << std::endl;
    }

    // --------------------------------------------------------------------------------
    Window::~Window()
    {
        if (m_window)
            glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    // --------------------------------------------------------------------------------
    void Window::BeginFrame()
    {
        m_mouse.scroll = 0.0f;
        m_mouse.dx = 0.0f;
        m_mouse.dy = 0.0f;
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
    }

    // --------------------------------------------------------------------------------
    void Window::EndFrame()
    {
        glfwSwapBuffers(m_window);
    }

    // --------------------------------------------------------------------------------
    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_window);
    }

    // --------------------------------------------------------------------------------
    void Window::GlfwKeyCallback(
        GLFWwindow* w,
        int key, int /*scancode*/, int action, int /*mods*/)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (self && self->m_key_cb)
            self->m_key_cb(key, action);
    }

    // --------------------------------------------------------------------------------
    void Window::GlfwMousePosCallback(GLFWwindow* w, double x, double y)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (!self) return;

        float fx = static_cast<float>(x);
        float fy = static_cast<float>(y);

        self->m_mouse.dx = fx - self->m_mouse.x;
        self->m_mouse.dy = fy - self->m_mouse.y;
        self->m_mouse.x = fx;
        self->m_mouse.y = fy;

        if (self->m_mouse_cb)
            self->m_mouse_cb(self->m_mouse);
    }

    // --------------------------------------------------------------------------------
    void Window::GlfwMouseBtnCallback(
        GLFWwindow* w,
        int button, int action, int /*mods*/)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (!self) return;

        bool pressed = (action == GLFW_PRESS);
        if (button == GLFW_MOUSE_BUTTON_LEFT)   self->m_mouse.left = pressed;
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)  self->m_mouse.right = pressed;
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE) self->m_mouse.middle = pressed;

        if (self->m_mouse_cb)
            self->m_mouse_cb(self->m_mouse);
    }

    // --------------------------------------------------------------------------------
    void Window::GlfwScrollCallback(GLFWwindow* w, double /*dx*/, double dy)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (!self) return;

        self->m_mouse.scroll = static_cast<float>(dy);
        if (self->m_scroll_cb)
            self->m_scroll_cb(self->m_mouse.scroll);
    }

    // --------------------------------------------------------------------------------
    void Window::GlfwResizeCallback(GLFWwindow* w, int width, int height)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (!self) return;

        self->m_width = width;
        self->m_height = height;
        glViewport(0, 0, width, height);

        if (self->m_resize_cb)
            self->m_resize_cb(width, height);
    }

} // namespace Toop