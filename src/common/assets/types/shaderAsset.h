#pragma once
#include "../asset.h"
#include "vulkan/vulkan_core.h"
#include <vector>
#include "utility/enumNameMap.h"

enum class ShaderType
{
	null,
	vertex,
	fragment,
	geometry,
	compute
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
	robin_hood::unordered_flat_map<std::string, UniformBufferData> uniforms;
	std::vector<ShaderVariableData> inputs;
	std::vector<ShaderVariableData> outputs;
    uint32_t runtimeID = -1;
	ShaderAsset();

	void serialize(OutputSerializer& s) const override;
	void deserialize(InputSerializer& s) override;
    VkShaderStageFlagBits vulkanShaderType() const;

#ifdef CLIENT
    void onDependenciesLoaded() override;
#endif
};