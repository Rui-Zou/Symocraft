//
// Created by Amo on 2022/6/15.
//

#include "core/window.h"
#include "core.h"

namespace SymoCraft
{

// User resize window callback func
// Static
    static void ResizeCallback(GLFWwindow* window_ptr, int new_width, int new_height)
    {
        Window* user_window = (Window*) glfwGetWindowUserPointer(window_ptr);
        user_window->width = new_width;
        user_window->height = new_height;
        glViewport(0, 0, new_width, new_height);
    }

// Static functions
    void Window::Init()
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);    // Multisample Anti-aliasing
    }

    void Window::Free()
    {
        // clean
        glfwTerminate();
    }


    Window* Window::Create(const char *window_title)
    {
        Window* res = new Window;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
            AmoLogger_Error("Failed to get primary monitor");
            return nullptr;
        }

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode)
        {
            AmoLogger_Error("Failed to get video mode of primary monitor");
            return nullptr;
        }
        AmoLogger_Info("Monitor size: %d, %d", mode->width, mode->height);

        // The smallest monitor size accepted is 800 * 600
        res->width = glm::clamp(mode->width / 2, 800, INT_MAX);
        res->height = glm::clamp(mode->height / 2, 600, INT_MAX);
        res->title = window_title;

        res->window_ptr = (void*) glfwCreateWindow(res->width, res->height, window_title, nullptr, nullptr);
        if (res->window_ptr == nullptr)
        {
            glfwTerminate();
            AmoLogger_Error("Failed to create a window. ");
            return res;
        }
        AmoLogger_Info("Window created. ");

        glfwSetWindowUserPointer((GLFWwindow*)res->window_ptr, (void*)res);
        res->MakeContextCurrent();

        int monitor_x, monitor_y;
        glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);

        int window_width, window_height;
        glfwGetWindowSize((GLFWwindow*)res->window_ptr, &window_width, &window_height);

        glfwSetWindowPos((GLFWwindow*)res->window_ptr,
                         monitor_x + (mode->width - window_width) /2,
                         monitor_y + (mode->height - window_height) / 2);

        res->SetVsync(true);

        // --------------------------------------------------------------------------------------------
        // glad : load all GLFW function pointers
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            AmoLogger_Error("Failed to load GLAD. ");
            return nullptr;
        }

        glViewport(0, 0, res->width, res->height);

        return res;
    }


    void Window::MakeContextCurrent()
    {
        glfwMakeContextCurrent((GLFWwindow*)window_ptr);
    }

    void Window::PollInt()
    {
        //
        glfwPollEvents();
    }

    void Window::SwapBuffers()
    {
        glfwSwapBuffers((GLFWwindow*)window_ptr);
    }

    bool Window::ShouldClose()
    {
        return glfwWindowShouldClose((GLFWwindow*)window_ptr);
    }





    void Window::Close()
    {
        glfwSetWindowShouldClose((GLFWwindow*)window_ptr, true);
    }

    void Window::Destroy()
    {
        glfwDestroyWindow((GLFWwindow*)window_ptr);
        window_ptr = nullptr;
    }

    void Window::SetCursorMode(CursorMode cursorMode)
    {
        int glfw_cursor_mode;
        switch (cursorMode)
        {
            case CursorMode::Lock:
                glfw_cursor_mode = GLFW_CURSOR_DISABLED;
                break;
            case CursorMode::Normal:
                glfw_cursor_mode = GLFW_CURSOR_NORMAL;
                break;
            case CursorMode::Hidden:
                glfw_cursor_mode = GLFW_CURSOR_HIDDEN;
                break;
            default:
                break;
        }
        glfwSetInputMode((GLFWwindow*)window_ptr, GLFW_CURSOR, glfw_cursor_mode);
    }

    void Window::SetVsync(bool on)
    {
        if (on)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
    }

    void Window::SetTitle(const char *new_title)
    {
        glfwSetWindowTitle((GLFWwindow*)window_ptr, new_title);
    }

    void Window::SetSize(int width, int height)
    {
        glfwSetWindowSize((GLFWwindow*)window_ptr, width, height);
    }

    float Window::GetAspectRatio() const
    {
        return (float) width / (float) height;
    }


}