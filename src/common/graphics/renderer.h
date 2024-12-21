#pragma once

#include <functional>

#include "glm/mat4x4.hpp"
#include "vulkan/vulkan.hpp"

namespace graphics {
    using RendererID = uint32_t;

    class Mesh;

    class Material;

    class RenderTexture;

    class SwapChain;

    struct RenderObject {
        Mesh* mesh;
        size_t primitive;

        bool operator==(const RenderObject&) const;
    };

    class Renderer {
      protected:
        VkRenderPass _renderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> _frameBuffers;
        SwapChain& _swapChain;
        RenderTexture* _target = nullptr;

        std::function<void()> _onDestroy;
        VkClearColorValue _clearColor;
        VkExtent2D _extent = {0, 0};
        bool _depthTexture = false;
        bool _shouldClear = true;

        void createRenderPass(VkFormat imageFormat, VkFormat depthImageFormat = VkFormat::VK_FORMAT_UNDEFINED);

        void createFrameBuffers(
            VkExtent2D size, const std::vector<VkImageView>& images, VkImageView depthTexture = VK_NULL_HANDLE);

        void startRenderPass(VkCommandBuffer cmdBuffer);

        void endRenderPass(VkCommandBuffer cmdBuffer);

        virtual void rebuild();

      public:
        uint8_t priority;

        Renderer(SwapChain& swapChain);

        virtual ~Renderer();

        bool targetingSwapChain();

        void setTargetAsSwapChain(bool depthTexture);

        void setTarget(RenderTexture* texture);

        void setClearColor(VkClearColorValue color);

        void dontClear();

        VkFramebuffer framebuffer(size_t index);

        VkExtent2D extent();

        virtual void render(VkCommandBuffer cmdBuffer) = 0;

        VkRenderPass renderPass();

        bool depthTexture();

        void clearTarget();
    };

    class CustomRenderer : public Renderer {
        std::function<void(VkCommandBuffer cmdBuffer)> _renderCallback;

      public:
        CustomRenderer(SwapChain& swapChain);

        void setRenderCallback(const std::function<void(VkCommandBuffer cmdBuffer)>& callback);

        void render(VkCommandBuffer cmdBuffer) override;
    };
} // namespace graphics
namespace std {
    template <> struct hash<graphics::RenderObject> {
        size_t operator()(const graphics::RenderObject&) const;
    };
} // namespace std
