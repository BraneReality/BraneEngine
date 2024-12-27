
#pragma once

#include <vulkan/vulkan.h>

#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "validationLayers.h"

namespace graphics
{

    class GraphicsDevice
    {

        VkDevice _device;
        VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;

        VkQueue _graphicsQueue;
        VkQueue _presentQueue;
        VkQueue _transferQueue;

        VkCommandPool _graphicsCommandPool;
        VkCommandPool _transferCommandPool;

        const std::vector<const char*> _deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

      public:
        struct QueueFamilyIndices
        {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;
            std::optional<uint32_t> transferFamily;

            bool isComplete()
            {
                return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
            }
        };

        struct SwapChainSupportDetails
        {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

      private:
        bool _validationLayersEnabled = false;
        QueueFamilyIndices _queueFamilyIndices;

        void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

        void createLogicalDevice();

        void createCommandPools();

        bool deviceHasExtensionSupport(VkPhysicalDevice device);

        int deviceValue(VkPhysicalDevice device, VkSurfaceKHR surface);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

      public:
        GraphicsDevice(VkInstance instance, VkSurfaceKHR surface);

        ~GraphicsDevice();

        VkDevice get();

        VkPhysicalDevice physicalDevice();

        SwapChainSupportDetails swapChainSupport(VkSurfaceKHR surface);

        QueueFamilyIndices queueFamilyIndices();

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        VkQueue graphicsQueue();

        VkQueue presentQueue();

        VkQueue transferQueue();

        VkCommandPool graphicsPool();

        VkCommandPool transferPool();

        VkPhysicalDeviceProperties properties();
    };

    extern GraphicsDevice* device;
} // namespace graphics