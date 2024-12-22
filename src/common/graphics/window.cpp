#include "window.h"
#include <iostream>
#include <stb_image.h>

namespace graphics {
    std::function<void()> Window::_onRefocus;

    Window::Window() { init(); }

    Window::~Window() { cleanup(); }

    void Window::createSurface(VkInstance instance, const VkAllocationCallbacks* allocator)
    {
        if(glfwCreateWindowSurface(instance, _window, nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void Window::destroySurface(VkInstance instance, const VkAllocationCallbacks* allocator)
    {
        vkDestroySurfaceKHR(instance, _surface, nullptr);
    }

    VkSurfaceKHR Window::surface() { return _surface; }

    void Window::init()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        _window = glfwCreateWindow(800, 600, "Brane Engine", nullptr, nullptr);
        assert(_window != NULL);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
        _framerate = vidMode->refreshRate;
        std::cout << "Using framrate of: " << _framerate << std::endl;

        int w, h, texChannels;
        unsigned char* pixels = stbi_load(ICON_IMAGE_PATH, &w, &h, &texChannels, STBI_rgb_alpha);
        if(pixels == nullptr) {
            throw std::runtime_error("failed to load icon");
        }
        GLFWimage icon{};
        icon.width = w;
        icon.height = h;
        icon.pixels = pixels;
        glfwSetWindowIcon(_window, 1, &icon);

        stbi_image_free(pixels);
    }

    void Window::update()
    {
        glfwPollEvents();
        glfwGetFramebufferSize(_window, &_size.x, &_size.y);
    }

    bool Window::closed() { return glfwWindowShouldClose(_window); }

    glm::ivec2 Window::size() { return _size; }

    void Window::cleanup()
    {

        glfwDestroyWindow(_window);

        glfwTerminate();
    }

    GLFWwindow* Window::window() { return _window; }

    int Window::framerate() const { return _framerate; }

    void Window::setTitle(std::string_view name) { glfwSetWindowTitle(_window, name.data()); }

    void Window::onRefocus(std::function<void()> onRefocus)
    {
        if(!_onRefocus)
            glfwSetWindowFocusCallback(_window, [](GLFWwindow* window, int focused) {
                if(focused)
                    _onRefocus();
            });
        _onRefocus = std::move(onRefocus);
    }
} // namespace graphics
