#pragma once

#include <vector>
#include "../asset.h"
#include "utility/enumNameMap.h"
#include "vulkan/vulkan_core.h"

enum class ShaderType : uint8_t
{
    null = 0,
    vertex = 1,
    fragment = 2,
    geometry = 3,
    compute = 4
};

struct ShaderVariableData
{
    enum Type
    {
        None,
        Boolean,
        Byte,
        UByte,
        Short,
        UShort,
        Int,
        UInt,
        Int64,
        UInt64,
        Half,
        Float,
        Double,
        Struct,
        Image,
        SampledImage,
        Sampler,
    };

    enum Layout
    {
        scalar,
        vec2,
        vec3,
        vec4,
        mat3,
        mat4
    };

    uint32_t location;
    std::string name;
    Type type;
    uint32_t size;
    uint32_t vecSize;
    uint32_t columns;

    Layout layout() const;

    static const EnumNameMap<Type> typeNames;
    static const EnumNameMap<Layout> layoutNames;
};

struct UniformBufferData
{
    uint32_t binding;
    std::string name;
    uint32_t size;
    std::vector<ShaderVariableData> members;
};

class ShaderAsset : public Asset
{
  public:
    ShaderType shaderType;
    std::vector<uint32_t> spirv;
    std::unordered_map<std::string, UniformBufferData> uniforms;
    std::unordered_map<std::string, UniformBufferData> buffers;
    std::vector<ShaderVariableData> inputs;
    std::vector<ShaderVariableData> outputs;

    ShaderAsset();

    void serialize(OutputSerializer& s) const override;

    void deserialize(InputSerializer& s) override;

    VkShaderStageFlagBits vulkanShaderType() const;

#ifdef CLIENT
    uint32_t runtimeID = -1;
    void onDependenciesLoaded() override;
#endif
};
