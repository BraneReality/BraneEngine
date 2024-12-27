#pragma once

#include <functional>
#include <stdexcept>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace graphics
{
    class Window
    {
      private:
        GLFWwindow* _window;
        glm::ivec2 _size;
        VkSurfaceKHR _surface;
        int _framerate;
        static std::function<void()> _onRefocus;

        void init();

        void cleanup();

      public:
        Window();

        ~Window();

        GLFWwindow* window();

        void createSurface(VkInstance instance, const VkAllocationCallbacks* allocator);

        void destroySurface(VkInstance instance, const VkAllocationCallbacks* allocator);

        VkSurfaceKHR surface();

        void update();

        bool closed();

        glm::ivec2 size();

        void setSize(glm::ivec2& newSize);

        int framerate() const;

        void setTitle(std::string_view name);

        void onRefocus(std::function<void()> onRefocus);
    };

} // namespace graphics
