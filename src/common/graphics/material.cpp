#include "material.h"
#include "assets/assetManager.h"
#include "assets/types/materialAsset.h"
#include "assets/types/shaderAsset.h"
#include "graphics.h"
#include "mesh.h"
#include "renderer.h"
#include "sceneRenderer.h"
#include "shader.h"

namespace graphics
{
    Material::Material(MaterialAsset* asset, VulkanRuntime* vkr) : _asset(asset)
    {
        auto* am = Runtime::getModule<AssetManager>();
        if(!asset->vertexShader.null())
        {
            AssetID id = asset->vertexShader;
            if(id.address().empty())
                id.setAddress(asset->id.address());
            _vertexShader = vkr->getShader(am->getAsset<ShaderAsset>(id)->runtimeID);
        }
        if(!asset->fragmentShader.null())
        {
            AssetID id = asset->fragmentShader;
            if(id.address().empty())
                id.setAddress(asset->id.address());
            _fragmentShader = vkr->getShader(am->getAsset<ShaderAsset>(id)->runtimeID);
        }

        _materialProperties.setFlags(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        if(!asset->serializedProperties.empty())
        {
            _materialProperties.realocate(asset->serializedProperties.size());
            _materialProperties.setData(asset->serializedProperties);
        }

        _transformBuffers.resize(vkr->swapChain()->size());
        for(auto& b : _transformBuffers)
        {
            b.setFlags(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            b.realocate(sizeof(glm::mat4) * 2);
        }

        uint32_t inputBindingIndex = 0;
        if(_vertexShader)
        {
            for(auto& input : _vertexShader->inputs())
            {
                VkFormat inputFormat = VK_FORMAT_R32_SFLOAT;
                switch(input.type)
                {
                    case ShaderVariableData::Float:
                        switch(input.layout())
                        {
                            case ShaderVariableData::scalar:
                                inputFormat = VK_FORMAT_R32_SFLOAT;
                                addBinding(inputBindingIndex, sizeof(float));
                                break;
                            case ShaderVariableData::vec2:
                                inputFormat = VK_FORMAT_R32G32_SFLOAT;
                                addBinding(inputBindingIndex, sizeof(glm::vec2));
                                break;
                            case ShaderVariableData::vec3:
                                inputFormat = VK_FORMAT_R32G32B32_SFLOAT;
                                addBinding(inputBindingIndex, sizeof(glm::vec3));
                                break;
                            case ShaderVariableData::vec4:
                                inputFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
                                addBinding(inputBindingIndex, sizeof(glm::vec4));
                                break;
                            default:
                                assert(false);
                        }
                        break;
                    default:
                        assert(false);
                }
                addAttribute(inputBindingIndex++, input.location, inputFormat, 0);
                _vertexBuffers.push_back(input.name);
            }
        }

        for(auto& textureBinding : asset->textures)
        {
            auto* image = am->getAsset<ImageAsset>(textureBinding.second);
            Texture* texture = vkr->getTexture(image->runtimeID);
            _textures.push_back({texture, textureBinding.first});
        }

        buildPipelineLayout(vkr->swapChain());
        initialize(vkr->swapChain()->size());
    }

    Material::Material(Material&& o)
    {
        _asset = o._asset;

        _geometryShader = o._geometryShader;
        _vertexShader = o._vertexShader;
        _fragmentShader = o._fragmentShader;

        _textures = std::move(o._textures);
        _bindings = std::move(o._bindings);
        _attributes = std::move(o._attributes);
        _transformBuffers = std::move(o._transformBuffers);
        _descriptorSets = std::move(_descriptorSets);
        _component = o._component;

        _descriptorSetLayout = o._descriptorSetLayout;
        _descriptorPool = o._descriptorPool;
        _pipelineLayout = o._pipelineLayout;
        o._descriptorSetLayout = VK_NULL_HANDLE;
        o._descriptorPool = VK_NULL_HANDLE;
        o._pipelineLayout = VK_NULL_HANDLE;
    }

    Material::~Material()
    {
        if(_pipelineLayout)
        {
            vkDestroyDescriptorSetLayout(device->get(), _descriptorSetLayout, nullptr);
            vkDestroyDescriptorPool(device->get(), _descriptorPool, nullptr);

            vkDestroyPipelineLayout(device->get(), _pipelineLayout, nullptr);
        }
    }

    Material& Material::operator=(Material&& o)
    {
        if(_pipelineLayout)
        {
            vkDestroyDescriptorSetLayout(device->get(), _descriptorSetLayout, nullptr);
            vkDestroyDescriptorPool(device->get(), _descriptorPool, nullptr);

            vkDestroyPipelineLayout(device->get(), _pipelineLayout, nullptr);
        }
        _asset = o._asset;

        _geometryShader = o._geometryShader;
        _vertexShader = o._vertexShader;
        _fragmentShader = o._fragmentShader;

        _textures = std::move(o._textures);
        _bindings = std::move(o._bindings);
        _attributes = std::move(o._attributes);
        _transformBuffers = std::move(o._transformBuffers);
        _descriptorSets = std::move(o._descriptorSets);
        _component = o._component;

        _descriptorSetLayout = o._descriptorSetLayout;
        _descriptorPool = o._descriptorPool;
        _pipelineLayout = o._pipelineLayout;
        o._descriptorSetLayout = VK_NULL_HANDLE;
        o._descriptorPool = VK_NULL_HANDLE;
        o._pipelineLayout = VK_NULL_HANDLE;
        return *this;
    }

    void Material::buildDescriptorSetVars(SwapChain* swapChain)
    {
        if(_descriptorSetLayout != VK_NULL_HANDLE)
            return;

        // Create layout
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        VkDescriptorSetLayoutBinding renderDataBinding{};
        renderDataBinding.binding = 0;
        renderDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        renderDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        renderDataBinding.descriptorCount = 1;
        bindings.push_back(renderDataBinding);

        VkDescriptorSetLayoutBinding instanceDataBinding{};
        instanceDataBinding.binding = 1;
        instanceDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        instanceDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        instanceDataBinding.descriptorCount = 1;
        bindings.push_back(instanceDataBinding);

        VkDescriptorSetLayoutBinding pointLightDataBinding{};
        pointLightDataBinding.binding = 2;
        pointLightDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pointLightDataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pointLightDataBinding.descriptorCount = 1;
        bindings.push_back(pointLightDataBinding);

        if(_asset->serializedProperties.size())
        {
            VkDescriptorSetLayoutBinding propertiesDataBinding{};
            propertiesDataBinding.binding = 3;
            propertiesDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            propertiesDataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            propertiesDataBinding.descriptorCount = 1;
            bindings.push_back(propertiesDataBinding);
        }

        for(size_t i = 0; i < _textures.size(); i++)
        {
            VkDescriptorSetLayoutBinding samplerLayoutBinding{};
            samplerLayoutBinding.binding = _textures[i].binding;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerLayoutBinding.pImmutableSamplers = nullptr;
            samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

            bindings.push_back(samplerLayoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if(vkCreateDescriptorSetLayout(device->get(), &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        // Create pool
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChain->size());
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChain->size());
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChain->size());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        poolInfo.maxSets = static_cast<uint32_t>(swapChain->size());

        if(vkCreateDescriptorPool(device->get(), &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void Material::buildPipelineLayout(SwapChain* swapChain)
    {
        if(_pipelineLayout)
            vkDestroyPipelineLayout(device->get(), _pipelineLayout, nullptr);

        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPushConstantRange push_constant{};
        push_constant.offset = 0;
        push_constant.size = sizeof(uint32_t);
        push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        buildDescriptorSetVars(swapChain);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &push_constant;

        if(vkCreatePipelineLayout(device->get(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    VkPipeline Material::pipeline(SceneRenderer* renderer) const
    {

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        if(_geometryShader != nullptr)
            shaderStages.push_back(_geometryShader->stageInfo());
        if(_vertexShader != nullptr)
            shaderStages.push_back(_vertexShader->stageInfo());
        if(_fragmentShader != nullptr)
            shaderStages.push_back(_fragmentShader->stageInfo());

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)_bindings.size();
        vertexInputInfo.pVertexBindingDescriptions = _bindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)_attributes.size();
        vertexInputInfo.pVertexAttributeDescriptions = _attributes.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = renderer->extent().width;
        viewport.height = renderer->extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = renderer->extent();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;          // Optional
        multisampling.pSampleMask = nullptr;            // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE;      // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {};  // Optional

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr; // Optional

        pipelineInfo.layout = _pipelineLayout;

        pipelineInfo.renderPass = renderer->renderPass();
        pipelineInfo.subpass = 0;

        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1;              // Optional

        VkPipeline pipeline;
        if(vkCreateGraphicsPipelines(device->get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        return pipeline;
    }

    VkPipelineLayout Material::pipelineLayout()
    {
        assert(_pipelineLayout != VK_NULL_HANDLE);
        return _pipelineLayout;
    }

    VkDescriptorSetLayout Material::descriptorLayout()
    {

        assert(_descriptorSetLayout != VK_NULL_HANDLE);
        return _descriptorSetLayout;
    }

    VkDescriptorPool Material::descriptorPool()
    {
        assert(_descriptorPool != VK_NULL_HANDLE);
        return _descriptorPool;
    }

    void Material::addBinding(uint32_t index, uint32_t stride)
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = index;
        bindingDescription.stride = stride;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        _bindings.push_back(bindingDescription);
    }

    void Material::addAttribute(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset)
    {
        VkVertexInputAttributeDescription attributeDescription;
        attributeDescription.binding = binding;
        attributeDescription.location = location;
        attributeDescription.format = format;
        attributeDescription.offset = offset;
        _attributes.push_back(attributeDescription);
    }

    void Material::initialize(size_t swapChainSize)
    {
        // Create sets
        if(_descriptorSets.size() < swapChainSize)
        {
            size_t currentSize = _descriptorSets.size();
            std::vector<VkDescriptorSetLayout> layouts(swapChainSize - currentSize, _descriptorSetLayout);
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = _descriptorPool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize - currentSize);
            allocInfo.pSetLayouts = layouts.data();

            _descriptorSets.resize(swapChainSize);
            if(vkAllocateDescriptorSets(device->get(), &allocInfo, &_descriptorSets[currentSize]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }
        }

        for(size_t i = 0; i < swapChainSize; i++)
        {
            std::vector<VkWriteDescriptorSet> descriptorWrites;

            VkDescriptorBufferInfo instanceDataInfo{};
            instanceDataInfo.buffer = _transformBuffers[i].get();
            instanceDataInfo.offset = 0;
            instanceDataInfo.range = VK_WHOLE_SIZE;

            VkWriteDescriptorSet instanceDataSet{};
            instanceDataSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            instanceDataSet.dstSet = _descriptorSets[i];
            instanceDataSet.dstBinding = 1;
            instanceDataSet.dstArrayElement = 0;
            instanceDataSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            instanceDataSet.descriptorCount = 1;
            instanceDataSet.pBufferInfo = &instanceDataInfo;
            descriptorWrites.push_back(instanceDataSet);

            std::vector<VkDescriptorImageInfo> descriptorImages;
            descriptorImages.reserve(
                _textures.size()); // This reserve makes it so we don't lose references due to resizes
            for(size_t image = 0; image < _textures.size(); image++)
            {
                VkDescriptorImageInfo newImageInfo{};
                newImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                newImageInfo.imageView = _textures[image].texture->view();
                newImageInfo.sampler = _textures[image].texture->sampler();
                descriptorImages.push_back(newImageInfo);

                VkWriteDescriptorSet newSet{};
                newSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                newSet.dstSet = _descriptorSets[i];
                newSet.dstBinding = _textures[image].binding;
                newSet.dstArrayElement = 0;
                newSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                newSet.descriptorCount = 1;
                newSet.pImageInfo = &descriptorImages[image];
                descriptorWrites.push_back(newSet);
            }

            vkUpdateDescriptorSets(
                device->get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    VkDescriptorSet const* Material::descriptorSet(size_t frame) const
    {
        assert(frame < _descriptorSets.size());
        return &_descriptorSets[frame];
    }

    const ComponentDescription* Material::component() const { return _component; }

    MaterialAsset* Material::asset() const { return _asset; }

    GraphicsBuffer& Material::transformBuffer(size_t frame)
    {
        assert(frame < _transformBuffers.size());
        return _transformBuffers[frame];
    }

    void Material::reallocateTransformBuffer(size_t frame, size_t newSize)
    {
        assert(frame < _transformBuffers.size());
        _transformBuffers[frame].realocate(newSize);

        VkDescriptorBufferInfo instanceDataInfo{};
        instanceDataInfo.buffer = _transformBuffers[frame].get();
        instanceDataInfo.offset = 0;
        instanceDataInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet instanceDataSet{};
        instanceDataSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        instanceDataSet.dstSet = _descriptorSets[frame];
        instanceDataSet.dstBinding = 1;
        instanceDataSet.dstArrayElement = 0;
        instanceDataSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        instanceDataSet.descriptorCount = 1;
        instanceDataSet.pBufferInfo = &instanceDataInfo;

        vkUpdateDescriptorSets(device->get(), 1, &instanceDataSet, 0, nullptr);
    }

    void Material::bindUniformBuffer(size_t frame, size_t binding, size_t uniformSize, GraphicsBuffer& buffer)
    {
        VkDescriptorBufferInfo instanceDataInfo{};
        instanceDataInfo.buffer = buffer.get();
        instanceDataInfo.offset = 0;
        instanceDataInfo.range = uniformSize;

        VkWriteDescriptorSet instanceDataSet{};
        instanceDataSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        instanceDataSet.dstSet = _descriptorSets[frame];
        instanceDataSet.dstBinding = binding;
        instanceDataSet.dstArrayElement = 0;
        instanceDataSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        instanceDataSet.descriptorCount = 1;
        instanceDataSet.pBufferInfo = &instanceDataInfo;

        vkUpdateDescriptorSets(device->get(), 1, &instanceDataSet, 0, nullptr);
    }

    void Material::bindPointLightBuffer(size_t frame, GraphicsBuffer& buffer)
    {
        VkDescriptorBufferInfo instanceDataInfo{};
        instanceDataInfo.buffer = buffer.get();
        instanceDataInfo.offset = 0;
        instanceDataInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet instanceDataSet{};
        instanceDataSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        instanceDataSet.dstSet = _descriptorSets[frame];
        instanceDataSet.dstBinding = 2;
        instanceDataSet.dstArrayElement = 0;
        instanceDataSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        instanceDataSet.descriptorCount = 1;
        instanceDataSet.pBufferInfo = &instanceDataInfo;

        vkUpdateDescriptorSets(device->get(), 1, &instanceDataSet, 0, nullptr);
    }

    void Material::bindProperties(size_t frame)
    {
        if(_materialProperties.size() == 0)
            return;
        VkDescriptorBufferInfo instanceDataInfo{};
        instanceDataInfo.buffer = _materialProperties.get();
        instanceDataInfo.offset = 0;
        instanceDataInfo.range = _materialProperties.size();

        VkWriteDescriptorSet instanceDataSet{};
        instanceDataSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        instanceDataSet.dstSet = _descriptorSets[frame];
        instanceDataSet.dstBinding = 3;
        instanceDataSet.dstArrayElement = 0;
        instanceDataSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        instanceDataSet.descriptorCount = 1;
        instanceDataSet.pBufferInfo = &instanceDataInfo;

        vkUpdateDescriptorSets(device->get(), 1, &instanceDataSet, 0, nullptr);
    }

    Shader* Material::vertexShader() const { return _vertexShader; }

    Shader* Material::fragmentShader() const { return _fragmentShader; }

    const std::vector<std::string>& Material::vertexBuffers() const { return _vertexBuffers; }

    GraphicsBuffer& Material::materialProperties() { return _materialProperties; }

    void Material::setMaterialProperties(const std::vector<uint8_t>& data)
    {
        if(data.size() > _materialProperties.size())
            _materialProperties.realocate(data.size());
        _materialProperties.setData(data);
    }
} // namespace graphics