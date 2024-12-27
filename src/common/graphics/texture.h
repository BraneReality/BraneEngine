#pragma once

#include "assets/types/imageAsset.h"
#include "graphicsBuffer.h"

#include <vulkan/vulkan.h>

#include <array>
#include <string>

namespace graphics
{
    using TextureID = uint32_t;
    extern const std::array<const char*, 1> textureFileExtensions;

    class Texture
    {
        ImageAsset* _asset;

        VkImage _textureImage;
        VkFormat _format;
        VkDeviceMemory _textureImageMemory;

        VkImageView _textureImageView;
        VkSampler _sampler = VK_NULL_HANDLE;

        void createTextureImageView();

        void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

        void copyBufferToImage(GraphicsBuffer& buffer);

      public:
        Texture(ImageAsset* asset);

        ~Texture();

        VkImage get();

        VkImageView view();

        VkSampler sampler();

        void update();
    };
} // namespace graphics