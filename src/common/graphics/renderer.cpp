#include "renderer.h"
#include "graphicsDevice.h"
#include "renderTarget.h"
#include "swapChain.h"
#include <ecs/nativeTypes/meshRenderer.h>

namespace graphics
{
    Renderer::Renderer(SwapChain& swapChain) : _swapChain(swapChain) {}

    Renderer::~Renderer()
    {
        if(_onDestroy)
            _onDestroy();
        vkDestroyRenderPass(device->get(), _renderPass, nullptr);
        for(auto framebuffer : _frameBuffers)
        {
            vkDestroyFramebuffer(device->get(), framebuffer, nullptr);
        }
    }

    void Renderer::createRenderPass(VkFormat imageFormat, VkFormat depthImageFormat)
    {
        if(_renderPass)
            vkDestroyRenderPass(device->get(), _renderPass, nullptr);
        std::vector<VkAttachmentDescription> attachments;

        _depthTexture = depthImageFormat != VK_FORMAT_UNDEFINED;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = imageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if(!_target)
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        else
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachments.push_back(colorAttachment);

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if(depthImageFormat)
        {
            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = depthImageFormat;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments.push_back(depthAttachment);
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        if(depthImageFormat)
            subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;

        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if(vkCreateRenderPass(device->get(), &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void Renderer::createFrameBuffers(VkExtent2D size, const std::vector<VkImageView>& images, VkImageView depthTexture)
    {
        _extent = size;
        for(auto buffer : _frameBuffers)
            vkDestroyFramebuffer(device->get(), buffer, nullptr);
        _frameBuffers.resize(0);
        _frameBuffers.resize(images.size());

        for(size_t i = 0; i < images.size(); i++)
        {
            std::vector<VkImageView> attachments = {images[i]};
            if(depthTexture)
                attachments.push_back(depthTexture);

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = _renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = size.width;
            framebufferInfo.height = size.height;
            framebufferInfo.layers = 1;

            if(vkCreateFramebuffer(device->get(), &framebufferInfo, nullptr, &_frameBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void Renderer::startRenderPass(VkCommandBuffer cmdBuffer)
    {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _renderPass;
        renderPassInfo.framebuffer = _frameBuffers[_swapChain.currentFrame()];

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = _extent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = _clearColor;
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void Renderer::endRenderPass(VkCommandBuffer cmdBuffer) { vkCmdEndRenderPass(cmdBuffer); }

    void Renderer::setTargetAsSwapChain(bool depthTexture)
    {
        _target = nullptr;
        if(depthTexture)
        {
            createRenderPass(_swapChain.imageFormat(), _swapChain.depthImageFormat());
            createFrameBuffers(_swapChain.extent(), _swapChain.getImages(), _swapChain.depthTexture());
        }
        else
        {
            createRenderPass(_swapChain.imageFormat());
            createFrameBuffers(_swapChain.extent(), _swapChain.getImages());
        }
        rebuild();
    }

    void Renderer::setTarget(RenderTexture* texture)
    {
        _target = texture;
        createRenderPass(texture->imageFormat(), texture->depthTextureFormat());
        createFrameBuffers(texture->size(), texture->images(), texture->depthTexture());
        rebuild();
    }

    VkRenderPass Renderer::renderPass() { return _renderPass; }

    void Renderer::setClearColor(VkClearColorValue color)
    {
        _shouldClear = true;
        _clearColor = color;
    }

    void Renderer::dontClear() { _shouldClear = false; }

    void Renderer::clearTarget()
    {
        for(auto buffer : _frameBuffers)
            vkDestroyFramebuffer(device->get(), buffer, nullptr);
        if(_renderPass)
            vkDestroyRenderPass(device->get(), _renderPass, nullptr);
        _frameBuffers.resize(0);
        _renderPass = VK_NULL_HANDLE;
    }

    bool Renderer::targetingSwapChain() { return !_target; }

    void Renderer::rebuild() {}

    VkExtent2D Renderer::extent() { return _extent; }

    bool Renderer::depthTexture() { return _depthTexture; }

    CustomRenderer::CustomRenderer(SwapChain& swapChain) : Renderer(swapChain) {}

    void CustomRenderer::setRenderCallback(const std::function<void(VkCommandBuffer)>& callback)
    {
        assert(callback);
        _renderCallback = callback;
    }

    void CustomRenderer::render(VkCommandBuffer cmdBuffer)
    {
        if(_renderPass == VK_NULL_HANDLE)
            return;
        startRenderPass(cmdBuffer);
        _renderCallback(cmdBuffer);
        endRenderPass(cmdBuffer);
    }

    bool RenderObject::operator==(const RenderObject& o) const { return mesh == o.mesh && primitive == o.primitive; }

} // namespace graphics

namespace std
{
    size_t hash<graphics::RenderObject>::operator()(const graphics::RenderObject& o) const
    {
        return hash<graphics::Mesh*>()(o.mesh) ^ o.primitive;
    }
} // namespace std
