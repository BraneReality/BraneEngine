#pragma once

#include "texture.h"

#include "swapChain.h"
#include <vulkan/vulkan.h>

class MaterialAsset;

class ComponentDescription;

namespace graphics
{
    class Shader;

    class VulkanRuntime;

    class SceneRenderer;

    class Material
    {
        MaterialAsset* _asset;
        Shader* _geometryShader = nullptr;
        Shader* _vertexShader = nullptr;
        Shader* _fragmentShader = nullptr;
        VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

        // Descriptors
        struct TextureBinding
        {
            Texture* texture;
            uint32_t binding;
        };

        std::vector<TextureBinding> _textures;
        std::vector<VkVertexInputBindingDescription> _bindings;
        std::vector<VkVertexInputAttributeDescription> _attributes;

        GraphicsBuffer _materialProperties;
        std::vector<GraphicsBuffer> _transformBuffers;

        VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> _descriptorSets;

        std::vector<std::string> _vertexBuffers;

        const ComponentDescription* _component;

        void buildDescriptorSetVars(SwapChain* swapChain);

      public:
        Material(MaterialAsset* asset, VulkanRuntime* vkr);

        Material(const Material&) = delete;

        Material(Material&&);

        ~Material();

        Material& operator=(Material&&);

        void addBinding(uint32_t binding, uint32_t stride);

        void addAttribute(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset);

        void buildPipelineLayout(SwapChain* swapChain);

        void initialize(size_t swapChainSize);

        GraphicsBuffer& transformBuffer(size_t frame);

        void reallocateTransformBuffer(size_t frame, size_t newSize);

        void bindProperties(size_t frame);

        void bindUniformBuffer(size_t frame, size_t binding, size_t uniformSize, GraphicsBuffer& buffer);

        void bindPointLightBuffer(size_t frame, GraphicsBuffer& buffer);

        const ComponentDescription* component() const;

        VkPipeline pipeline(SceneRenderer* renderer) const;

        VkPipelineLayout pipelineLayout();

        VkDescriptorSetLayout descriptorLayout();

        VkDescriptorPool descriptorPool();

        GraphicsBuffer& materialProperties();

        void setMaterialProperties(const std::vector<uint8_t>& data);

        VkDescriptorSet const* descriptorSet(size_t frame) const;

        MaterialAsset* asset() const;

        Shader* vertexShader() const;

        Shader* fragmentShader() const;

        const std::vector<std::string>& vertexBuffers() const;
    };

} // namespace graphics