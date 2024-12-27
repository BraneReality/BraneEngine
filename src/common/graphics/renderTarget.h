//
// Created by eli on 5/25/2022.
//

#ifndef BRANEENGINE_RENDERTARGET_H
#define BRANEENGINE_RENDERTARGET_H

#include <vector>
#include <vulkan/vulkan.h>

namespace graphics
{
    class SwapChain;

    class RenderTexture
    {
        SwapChain& _sc;

        VkDeviceMemory _imageMemory;
        std::vector<VkImage> _images;
        std::vector<VkImageView> _imageViews;
        VkSampler _sampler;

        VkFormat _format;
        VkImage _depthTexture = VK_NULL_HANDLE;
        VkImageView _depthTextureView = VK_NULL_HANDLE;
        VkFormat _depthFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D _size;

      public:
        RenderTexture(VkExtent2D size, bool depthTexture, SwapChain& swapChain);

        ~RenderTexture();

        VkFormat imageFormat();

        VkFormat depthTextureFormat();

        VkExtent2D size();

        const std::vector<VkImageView>& images();

        VkImageView depthTexture();

        VkSampler sampler();
    };
} // namespace graphics

#endif // BRANEENGINE_RENDERTARGET_H
