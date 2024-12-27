//
// Created by eli on 5/25/2022.
//

#include "renderTarget.h"
#include <exception>
#include "graphicsDevice.h"
#include "swapChain.h"

VkFormat graphics::RenderTexture::imageFormat() { return _format; }

VkFormat graphics::RenderTexture::depthTextureFormat() { return _depthFormat; }

VkExtent2D graphics::RenderTexture::size() { return _size; }

const std::vector<VkImageView>& graphics::RenderTexture::images() { return _imageViews; }

VkImageView graphics::RenderTexture::depthTexture() { return _depthTextureView; }

graphics::RenderTexture::RenderTexture(VkExtent2D size, bool depthTexture, graphics::SwapChain& swapChain)
    : _sc(swapChain)
{
    _size = size;
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {size.width, size.height, 0};
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;

    _format = swapChain.imageFormat();
    imageInfo.format = _format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    _images.resize(swapChain.size());
    _imageViews.resize(swapChain.size());
    for(size_t i = 0; i < swapChain.size(); ++i)
    {
        if(vkCreateImage(device->get(), &imageInfo, nullptr, &_images[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture!");
        }
    }

    if(depthTexture)
    {
        VkImageCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.extent = {size.width, size.height, 0};
        createInfo.extent.depth = 1;
        createInfo.mipLevels = 1;
        createInfo.arrayLayers = 1;

        _depthFormat = swapChain.depthImageFormat();
        createInfo.format = _depthFormat;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.flags = 0;

        if(vkCreateImage(device->get(), &createInfo, nullptr, &_depthTexture) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image!");
        }
    }

    VkMemoryRequirements imageReqs;
    vkGetImageMemoryRequirements(device->get(), _images[0], &imageReqs);
    VkDeviceSize alignedImageSize = imageReqs.size + (imageReqs.alignment - imageReqs.size % imageReqs.alignment);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = alignedImageSize * _images.size();

    VkDeviceSize depthTexOffset;
    if(depthTexture)
    {
        VkMemoryRequirements depthReqs;
        vkGetImageMemoryRequirements(device->get(), _depthTexture, &depthReqs);
        depthTexOffset =
            allocInfo.allocationSize + depthReqs.alignment - allocInfo.allocationSize % depthReqs.alignment;
        allocInfo.allocationSize = depthTexOffset + depthReqs.size;
        imageReqs.memoryTypeBits |= depthReqs.memoryTypeBits;
    }
    allocInfo.memoryTypeIndex = device->findMemoryType(imageReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if(vkAllocateMemory(device->get(), &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate texture memory!");
    }

    for(size_t i = 0; i < swapChain.size(); ++i)
    {
        vkBindImageMemory(device->get(), _images[i], _imageMemory, alignedImageSize * i);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = _format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(device->get(), &viewInfo, nullptr, &_imageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }
    }

    if(depthTexture)
    {
        vkBindImageMemory(device->get(), _depthTexture, _imageMemory, depthTexOffset);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _depthTexture;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = _depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(device->get(), &viewInfo, nullptr, &_depthTextureView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = device->properties().limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if(vkCreateSampler(device->get(), &samplerInfo, nullptr, &_sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

VkSampler graphics::RenderTexture::sampler() { return _sampler; }

graphics::RenderTexture::~RenderTexture()
{
    for(size_t i = 0; i < _images.size(); ++i)
    {
        vkDestroyImageView(device->get(), _imageViews[i], nullptr);
        vkDestroyImage(device->get(), _images[i], nullptr);
    }
    if(_depthTexture)
    {
        vkDestroyImageView(device->get(), _depthTextureView, nullptr);
        vkDestroyImage(device->get(), _depthTexture, nullptr);
    }
    vkFreeMemory(device->get(), _imageMemory, nullptr);
    vkDestroySampler(device->get(), _sampler, nullptr);
}
