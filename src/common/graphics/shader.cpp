#include "shader.h"
#include "assets/types/shaderAsset.h"
#include "graphicsDevice.h"

namespace graphics
{
    Shader::Shader(ShaderAsset* asset) : _asset(asset)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = _asset->spirv.size() * sizeof(uint32_t);
        createInfo.pCode = _asset->spirv.data();

        if(vkCreateShaderModule(device->get(), &createInfo, nullptr, &_shader) != VK_SUCCESS)
        {
            throw std::runtime_error("Could not create shader module!");
        }
    }

    Shader::Shader(Shader&& o)
    {
        _asset = o._asset;
        _shader = o._shader;
        o._shader = VK_NULL_HANDLE;
    }

    Shader::~Shader()
    {
        if(_shader)
            vkDestroyShaderModule(device->get(), _shader, nullptr);
    }

    Shader& Shader::operator=(Shader&& o)
    {
        if(_shader)
            vkDestroyShaderModule(device->get(), _shader, nullptr);
        _asset = o._asset;
        _shader = o._shader;
        o._shader = VK_NULL_HANDLE;
        return *this;
    }

    VkShaderModule Shader::get() { return _shader; }

    VkShaderStageFlagBits Shader::type() { return _asset->vulkanShaderType(); }

    VkPipelineShaderStageCreateInfo Shader::stageInfo()
    {
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = _asset->vulkanShaderType();
        shaderStageInfo.module = _shader;
        shaderStageInfo.pName = "main";

        return shaderStageInfo;
    }

    ShaderAsset* Shader::asset() const { return _asset; }

    std::vector<ShaderVariableData>& Shader::inputs() const { return _asset->inputs; }

} // namespace graphics