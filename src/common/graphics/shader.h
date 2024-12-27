#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include "assets/types/shaderAsset.h"
#include <vulkan/vulkan.h>

namespace graphics
{
    using ShaderID = uint64_t;

    class Shader
    {
        ShaderAsset* _asset;
        VkShaderModule _shader;

      public:
        Shader(ShaderAsset* asset);

        Shader(const Shader&) = delete;

        Shader(Shader&&);

        Shader& operator=(Shader&&);

        ~Shader();

        ShaderAsset* asset() const;

        VkShaderModule get();

        VkShaderStageFlagBits type();

        VkPipelineShaderStageCreateInfo stageInfo();

        std::vector<ShaderVariableData>& inputs() const;
    };

} // namespace graphics