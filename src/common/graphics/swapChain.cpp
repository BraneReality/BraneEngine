#include "swapChain.h"

#include "graphicsDevice.h"

namespace graphics
{

    void SwapChain::createSwapChain()
    {
        auto swapChainSupport = device->swapChainSupport(_window->surface());

        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);

        _imageFormat = surfaceFormat.format;
        _extent = chooseExtent(swapChainSupport.capabilities);

        uint32_t imageCount = std::max<uint32_t>(swapChainSupport.capabilities.minImageCount + 1, 2);
        if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
            imageCount = swapChainSupport.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = _window->surface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = _imageFormat;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = chooseExtent(swapChainSupport.capabilities);
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto indices = device->queueFamilyIndices();
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if(indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;     // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if(vkCreateSwapchainKHR(device->get(), &createInfo, nullptr, &_swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }
        vkGetSwapchainImagesKHR(device->get(), _swapChain, &imageCount, nullptr);
        _images.resize(imageCount);
        _size = imageCount;
        _currentFrame = _size - 1;
        vkGetSwapchainImagesKHR(device->get(), _swapChain, &imageCount, _images.data());
    }

    void SwapChain::createImageViews()
    {
        _imageViews.resize(_images.size());
        for(size_t i = 0; i < _images.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = _images[i];

            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = _imageFormat;

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if(vkCreateImageView(device->get(), &createInfo, nullptr, &_imageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    void SwapChain::createDepthResources()
    {
        _depthImageFormat = findDepthFormat();
        VkImageCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.extent.width = _extent.width;
        createInfo.extent.height = _extent.height;
        createInfo.extent.depth = 1;
        createInfo.mipLevels = 1;
        createInfo.arrayLayers = 1;

        createInfo.format = _depthImageFormat;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.flags = 0;

        if(vkCreateImage(device->get(), &createInfo, nullptr, &_depthImage) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device->get(), _depthImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex =
            device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if(vkAllocateMemory(device->get(), &allocInfo, nullptr, &_depthImageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device->get(), _depthImage, _depthImageMemory, 0);

        // Now create the view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = _depthImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(device->get(), &viewInfo, nullptr, &_depthImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }
    }

    VkExtent2D SwapChain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if(capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        else
        {
            glm::vec2 size = _window->size();

            VkExtent2D actualExtent = {static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)};

            actualExtent.width =
                std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height =
                std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        // Find the best format
        for(const auto& availableFormat : availableFormats)
        {
            if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
               availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for(const auto& availablePresentMode : availablePresentModes)
        {
            if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkFormat SwapChain::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                            VkImageTiling tiling,
                                            VkFormatFeatureFlags features)
    {
        for(VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(device->physicalDevice(), format, &props);

            if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }
        throw std::runtime_error("failed to find compatible image format!");
    }

    VkFormat SwapChain::findDepthFormat()
    {
        return findSupportedFormat({VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    SwapChain::SwapChain(Window* window)
    {
        _window = window;
        createSwapChain();
        createImageViews();
        createDepthResources();

        _imageAvailableSemaphores.resize(_size);
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for(size_t i = 0; i < _size; i++)
        {
            if(vkCreateSemaphore(device->get(), &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create semaphores for a frame!");
            }
        }
    }

    SwapChain::~SwapChain()
    {
        for(auto s : _imageAvailableSemaphores)
            vkDestroySemaphore(device->get(), s, nullptr);

        vkDestroyImageView(device->get(), _depthImageView, nullptr);
        vkDestroyImage(device->get(), _depthImage, nullptr);
        vkFreeMemory(device->get(), _depthImageMemory, nullptr);

        for(auto imageView : _imageViews)
        {
            vkDestroyImageView(device->get(), imageView, nullptr);
        }

        vkDestroySwapchainKHR(device->get(), _swapChain, nullptr);
    }

    VkSwapchainKHR SwapChain::get() { return _swapChain; }

    VkResult SwapChain::acquireNextImage()
    {
        _currentSemaphore = (_currentSemaphore + 1) % _imageAvailableSemaphores.size();
        return vkAcquireNextImageKHR(device->get(),
                                     _swapChain,
                                     UINT64_MAX,
                                     _imageAvailableSemaphores[_currentSemaphore],
                                     VK_NULL_HANDLE,
                                     &_currentFrame);
    }

    size_t SwapChain::size() { return _size; }

    VkExtent2D SwapChain::extent() { return _extent; }

    VkFormat SwapChain::imageFormat() { return _imageFormat; }

    VkFormat SwapChain::depthImageFormat() { return _depthImageFormat; }

    const std::vector<VkImageView>& SwapChain::getImages() { return _imageViews; }

    const uint32_t& SwapChain::currentFrame() { return _currentFrame; }

    uint32_t SwapChain::nextFrame() { return (_currentFrame + 1) % _images.size(); }

    VkSemaphore SwapChain::currentSemaphore() { return _imageAvailableSemaphores[_currentSemaphore]; }

    VkImageView SwapChain::depthTexture() { return _depthImageView; }

    void SwapChain::resize()
    {
        vkDestroyImageView(device->get(), _depthImageView, nullptr);
        vkDestroyImage(device->get(), _depthImage, nullptr);
        vkFreeMemory(device->get(), _depthImageMemory, nullptr);

        for(auto imageView : _imageViews)
        {
            vkDestroyImageView(device->get(), imageView, nullptr);
        }

        vkDestroySwapchainKHR(device->get(), _swapChain, nullptr);

        createSwapChain();
        createImageViews();
        createDepthResources();
        _currentFrame = 0;
        _currentSemaphore = 0;
    }
} // namespace graphics