#include "shaderAsset.h"
#include "graphics/graphics.h"
#include "runtime/runtime.h"
#include "utility/enumNameMap.h"
#include "utility/serializedData.h"

OutputSerializer operator<<(OutputSerializer s, const ShaderVariableData& var)
{
    s << var.location << var.name << var.type << var.size << var.vecSize << var.columns;
    return s;
}

InputSerializer operator>>(InputSerializer s, ShaderVariableData& var)
{
    s >> var.location >> var.name >> var.type >> var.size >> var.vecSize >> var.columns;
    return s;
}

OutputSerializer operator<<(OutputSerializer s, const std::vector<ShaderVariableData>& attributes)
{
    s << static_cast<uint16_t>(attributes.size());
    for(auto& a : attributes)
        s << a;
    return s;
}

InputSerializer operator>>(InputSerializer s, std::vector<ShaderVariableData>& attributes)
{
    uint16_t size;
    s >> size;
    attributes.resize(size);
    for(uint16_t i = 0; i < size; ++i)
        s >> attributes[i];
    return s;
}

OutputSerializer operator<<(OutputSerializer s, const std::unordered_map<std::string, UniformBufferData>& buffers)
{
    s << static_cast<uint16_t>(buffers.size());
    for(auto& buffer : buffers)
    {
        auto& b = buffer.second;
        s << b.binding << b.name << b.size << static_cast<uint16_t>(b.members.size());
        for(auto& m : b.members)
            s << m;
    }
    return s;
}

InputSerializer operator>>(InputSerializer s, std::unordered_map<std::string, UniformBufferData>& buffers)
{
    uint16_t size;
    s >> size;
    for(uint16_t b = 0; b < size; ++b)
    {
        UniformBufferData buffer;
        uint16_t memberCount;
        s >> buffer.binding >> buffer.name >> buffer.size >> memberCount;
        buffer.members.resize(memberCount);
        for(uint16_t m = 0; m < memberCount; ++m)
            s >> buffer.members[m];
        buffers.insert({buffer.name, buffer});
    }
    return s;
}

void ShaderAsset::serialize(OutputSerializer& s) const
{
    Asset::serialize(s);
    s << shaderType << spirv << uniforms << inputs << outputs;
}

void ShaderAsset::deserialize(InputSerializer& s)
{
    Asset::deserialize(s);
    s >> shaderType >> spirv >> uniforms >> inputs >> outputs;
}

ShaderAsset::ShaderAsset() { type.set(AssetType::Type::shader); }

#ifdef CLIENT
void ShaderAsset::onDependenciesLoaded()
{
    auto* vkr = Runtime::getModule<graphics::VulkanRuntime>();
    runtimeID = vkr->addAsset(this);
}
#endif

VkShaderStageFlagBits ShaderAsset::vulkanShaderType() const
{
    switch(shaderType)
    {

        case ShaderType::vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderType::fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderType::geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderType::compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
    }
    assert(false && "Unreachable");
    return (VkShaderStageFlagBits)0;
}

ShaderVariableData::Layout ShaderVariableData::layout() const
{
    if(vecSize == 1)
        return scalar;
    if(columns == 1)
    {
        if(vecSize == 3)
            return vec3;
        if(vecSize == 4)
            return vec4;
        if(vecSize == 2)
            return vec2;
    }
    if(columns == 4)
        return mat4;
    if(columns == 3)
        return mat3;
    return scalar;
}

const EnumNameMap<ShaderVariableData::Type> ShaderVariableData::typeNames({
    {ShaderVariableData::None, "None"},
    {ShaderVariableData::Boolean, "Bool"},
    {ShaderVariableData::Byte, "Byte"},
    {ShaderVariableData::UByte, "UByte"},
    {ShaderVariableData::Short, "Short"},
    {ShaderVariableData::UShort, "UShort"},
    {ShaderVariableData::Int, "Int"},
    {ShaderVariableData::UInt, "UInt"},
    {ShaderVariableData::Int64, "Int64"},
    {ShaderVariableData::UInt64, "UInt64"},
    {ShaderVariableData::Half, "Half"},
    {ShaderVariableData::Float, "Float"},
    {ShaderVariableData::Double, "Double"},
    {ShaderVariableData::Struct, "Struct"},
    {ShaderVariableData::Image, "Image"},
    {ShaderVariableData::SampledImage, "SampledImage"},
    {ShaderVariableData::Sampler, "Sampler"},
});

const EnumNameMap<ShaderVariableData::Layout> ShaderVariableData::layoutNames({
    {ShaderVariableData::scalar, "scalar"},
    {ShaderVariableData::vec2, "vec2"},
    {ShaderVariableData::vec3, "vec3"},
    {ShaderVariableData::vec4, "vec4"},
    {ShaderVariableData::mat3, "mat3"},
    {ShaderVariableData::mat4, "mat4"},
});
