#include "graphicsDevice.h"

namespace graphics {
    GraphicsDevice* device;

    void GraphicsDevice::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if(deviceCount == 0)
            throw std::runtime_error("GPU(s) does not support vulkan");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::multimap<int, VkPhysicalDevice> candidates;

        for(const auto& device : devices) {
            int devVal = deviceValue(device, surface);
            candidates.insert(std::make_pair(devVal, device));
        }

        if(candidates.rbegin()->first > 0) {
            _physicalDevice = candidates.rbegin()->second;
        }
        else
            throw std::runtime_error("failed to find a suitable GPU!");
    }

    void GraphicsDevice::createLogicalDevice()
    {
        QueueFamilyIndices& indices = _queueFamilyIndices;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value()};

        float queuePriority = 1.0f;
        for(uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{
            .vertexPipelineStoresAndAtomics = VK_TRUE,
            .fragmentStoresAndAtomics = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
        };

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = _deviceExtensions.data();

        VkPhysicalDeviceVulkan11Features features{};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        features.shaderDrawParameters = true;
        createInfo.pNext = &features;

        if(_validationLayersEnabled) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if(vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vulkan logical device!");
        }

        vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
        vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_presentQueue);
        vkGetDeviceQueue(_device, indices.transferFamily.value(), 0, &_transferQueue);
    }

    bool GraphicsDevice::deviceHasExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(_deviceExtensions.begin(), _deviceExtensions.end());

        for(const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    int GraphicsDevice::deviceValue(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        // Check to make sure is has the queues we need
        if(!findQueueFamilies(device, surface).isComplete())
            return 0;

        if(!deviceHasExtensionSupport(device))
            return 0;

        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        if(swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
            return 0;

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        if(!supportedFeatures.samplerAnisotropy)
            return 0;

        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        int score = 0;

        // Discrete GPUs have a significant performance advantage
        if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        return score;
    }

    GraphicsDevice::QueueFamilyIndices GraphicsDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for(int i = 0; i < queueFamilies.size(); i++) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if(presentSupport) {
                indices.presentFamily = i;
            }
            if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }
            else if(queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                indices.transferFamily = i;
            }
            if(indices.isComplete())
                break;
        }

        return indices;
    }

    GraphicsDevice::SwapChainSupportDetails
    GraphicsDevice::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if(formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if(presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    GraphicsDevice::GraphicsDevice(VkInstance instance, VkSurfaceKHR surface)
    {
        pickPhysicalDevice(instance, surface);
        _queueFamilyIndices = findQueueFamilies(_physicalDevice, surface);
        createLogicalDevice();
        device = this;
        createCommandPools();
    }

    GraphicsDevice::~GraphicsDevice()
    {
        vkDestroyCommandPool(_device, _graphicsCommandPool, nullptr);
        vkDestroyCommandPool(_device, _transferCommandPool, nullptr);
        vkDestroyDevice(_device, nullptr);
    }

    GraphicsDevice::QueueFamilyIndices GraphicsDevice::queueFamilyIndices() { return _queueFamilyIndices; }

    void GraphicsDevice::createCommandPools()
    {
        auto queueFamilyIndices = _queueFamilyIndices;

        VkCommandPoolCreateInfo graphicsPoolInfo{};
        graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        graphicsPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if(vkCreateCommandPool(_device, &graphicsPoolInfo, nullptr, &_graphicsCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }

        VkCommandPoolCreateInfo transferPoolInfo{};
        transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        transferPoolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();
        transferPoolInfo.flags = 0; // Optional

        if(vkCreateCommandPool(_device, &transferPoolInfo, nullptr, &_transferCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create transfer command pool!");
        }
    }

    uint32_t GraphicsDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

        for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkQueue GraphicsDevice::graphicsQueue() { return _graphicsQueue; }

    VkQueue GraphicsDevice::presentQueue() { return _presentQueue; }

    VkQueue GraphicsDevice::transferQueue() { return _transferQueue; }

    VkCommandPool GraphicsDevice::graphicsPool() { return _graphicsCommandPool; }

    VkCommandPool GraphicsDevice::transferPool() { return _transferCommandPool; }

    VkPhysicalDeviceProperties GraphicsDevice::properties()
    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(_physicalDevice, &props);
        return props;
    }

    VkDevice GraphicsDevice::get() { return _device; }

    VkPhysicalDevice GraphicsDevice::physicalDevice() { return _physicalDevice; }

    GraphicsDevice::SwapChainSupportDetails GraphicsDevice::swapChainSupport(VkSurfaceKHR surface)
    {
        return querySwapChainSupport(_physicalDevice, surface);
    }
} // namespace graphics
