//
// Created by eli on 8/31/2022.
//

#include "shaderCompiler.h"
#include "assets/types/shaderAsset.h"
#include "runtime/runtime.h"
#include "file_includer.h"
#include "spirv_cross/spirv_cross.hpp"
#include <filesystem>

ShaderCompiler::ShaderCompiler() {}

bool ShaderCompiler::compileShader(
    const std::string& glsl,
    ShaderType type,
    std::vector<uint32_t>& spirv,
    shaderc_util::FileFinder& fileFinder,
    bool optimize)
{
    shaderc_shader_kind kind;
    switch(type) {

    case ShaderType::vertex:
        kind = shaderc_glsl_vertex_shader;
        break;
    case ShaderType::fragment:
        kind = shaderc_glsl_fragment_shader;
        break;
    case ShaderType::geometry:
        kind = shaderc_glsl_geometry_shader;
        break;
    case ShaderType::compute:
        kind = shaderc_glsl_compute_shader;
        break;
    default:
        Runtime::error("Tried to compile unknown shader type!");
        assert(false);
        return false;
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(optimize ? shaderc_optimization_level_performance : shaderc_optimization_level_zero);
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    options.SetTargetSpirv(shaderc_spirv_version_1_5);
    options.SetIncluder(std::make_unique<glslc::FileIncluder>(&fileFinder));

    auto result = compiler.CompileGlslToSpv(glsl, kind, "shader line", options);

    if(result.GetCompilationStatus() != shaderc_compilation_status_success) {
        Runtime::error("Could not compile shader:\n" + result.GetErrorMessage());
        return false;
    }
    spirv = {result.begin(), result.end()};
    return true;
}

ShaderVariableData::Type typeFromSpirvType(spirv_cross::SPIRType::BaseType type)
{
    switch(type) {
    case spirv_cross::SPIRType::Boolean:
        return ShaderVariableData::Type::Boolean;
    case spirv_cross::SPIRType::SByte:
        return ShaderVariableData::Type::Byte;
    case spirv_cross::SPIRType::UByte:
        return ShaderVariableData::Type::UByte;
    case spirv_cross::SPIRType::Short:
        return ShaderVariableData::Type::Short;
    case spirv_cross::SPIRType::UShort:
        return ShaderVariableData::Type::UShort;
    case spirv_cross::SPIRType::Int:
        return ShaderVariableData::Type::Int;
    case spirv_cross::SPIRType::UInt:
        return ShaderVariableData::Type::UInt;
    case spirv_cross::SPIRType::Int64:
        return ShaderVariableData::Type::Int64;
    case spirv_cross::SPIRType::UInt64:
        return ShaderVariableData::Type::UInt64;
    case spirv_cross::SPIRType::Half:
        return ShaderVariableData::Type::Half;
    case spirv_cross::SPIRType::Float:
        return ShaderVariableData::Type::Float;
    case spirv_cross::SPIRType::Double:
        return ShaderVariableData::Type::Double;
    case spirv_cross::SPIRType::Struct:
        return ShaderVariableData::Type::Struct;
    case spirv_cross::SPIRType::Image:
        return ShaderVariableData::Type::Image;
    case spirv_cross::SPIRType::SampledImage:
        return ShaderVariableData::Type::SampledImage;
    case spirv_cross::SPIRType::Sampler:
        return ShaderVariableData::Type::Sampler;
    default:
        return ShaderVariableData::Type::None;
    }
}

bool ShaderCompiler::extractAttributes(
    const std::string& glsl, ShaderType shaderType, shaderc_util::FileFinder& fileFinder, ShaderAttributes& attributes)
{
    // We need to compile without optimization to be able to extract variable names
    std::vector<uint32_t> spirv;
    if(!compileShader(glsl, shaderType, spirv, fileFinder, false))
        return false;

    spirv_cross::Compiler compiler(std::move(spirv));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    for(auto& uniformBuffer : resources.uniform_buffers) {
        UniformBufferData ub{};
        ub.name = uniformBuffer.name;
        ub.binding = compiler.get_decoration(uniformBuffer.id, spv::DecorationBinding);
        auto& type = compiler.get_type(uniformBuffer.base_type_id);
        assert(type.basetype == spirv_cross::SPIRType::Struct);
        ub.size = compiler.get_declared_struct_size(type);
        for(size_t i = 0; i < type.member_types.size(); ++i) {
            auto& memberType = compiler.get_type(type.member_types[i]);
            ShaderVariableData memberData;
            memberData.location = compiler.get_member_decoration(uniformBuffer.base_type_id, i, spv::DecorationOffset);
            memberData.name = compiler.get_member_name(uniformBuffer.base_type_id, i);
            memberData.size = compiler.get_declared_struct_member_size(type, i);
            memberData.type = typeFromSpirvType(memberType.basetype);
            memberData.vecSize = memberType.vecsize;
            memberData.columns = memberType.columns;
            ub.members.push_back(memberData);
        }

        attributes.uniforms.push_back(ub);
    }
    for(auto& uniformBuffer : resources.storage_buffers) {
        UniformBufferData ub{};
        ub.name = uniformBuffer.name;
        ub.binding = compiler.get_decoration(uniformBuffer.id, spv::DecorationBinding);
        auto& type = compiler.get_type(uniformBuffer.base_type_id);
        assert(type.basetype == spirv_cross::SPIRType::Struct);
        ub.size = compiler.get_declared_struct_size(type);
        for(size_t i = 0; i < type.member_types.size(); ++i) {
            auto& memberType = compiler.get_type(type.member_types[i]);
            ShaderVariableData memberData;
            memberData.location = compiler.get_member_decoration(uniformBuffer.base_type_id, i, spv::DecorationOffset);
            memberData.name = compiler.get_member_name(uniformBuffer.base_type_id, i);
            memberData.size = compiler.get_declared_struct_member_size(type, i);
            memberData.type = typeFromSpirvType(memberType.basetype);
            memberData.vecSize = memberType.vecsize;
            memberData.columns = memberType.columns;
            ub.members.push_back(memberData);
        }

        attributes.buffers.push_back(ub);
    }
    for(auto& sampler : resources.sampled_images) {
        ShaderVariableData samp{};
        samp.name = sampler.name;
        samp.location = compiler.get_decoration(sampler.id, spv::DecorationBinding);
        attributes.samplers.push_back(samp);
    }
    for(auto& stageInput : resources.stage_inputs) {
        ShaderVariableData var;
        var.location = compiler.get_decoration(stageInput.id, spv::DecorationLocation);
        var.name = stageInput.name;
        auto& type = compiler.get_type(stageInput.base_type_id);
        var.type = typeFromSpirvType(type.basetype);
        var.size = type.width;
        var.vecSize = type.vecsize;
        var.columns = type.columns;
        attributes.inputVariables.push_back(var);
    }
    for(auto& stageOutput : resources.stage_outputs) {
        ShaderVariableData var;
        var.location = compiler.get_decoration(stageOutput.id, spv::DecorationLocation);
        var.name = stageOutput.name;
        auto& type = compiler.get_type(stageOutput.base_type_id);
        var.type = typeFromSpirvType(type.basetype);
        var.size = type.width;
        var.vecSize = type.vecsize;
        var.columns = type.columns;
        attributes.outputVariables.push_back(var);
    }

    return true;
}

shaderc_util::FileFinder ShaderCompiler::defaultFinder()
{
    shaderc_util::FileFinder finder;
    finder.search_path().push_back((std::filesystem::current_path() / "defaultAssets" / "shaders").string());
    return std::move(finder);
}
