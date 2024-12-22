//
// Created by eli on 8/31/2022.
//

#include "shaderCompiler.h"
#include "assets/types/shaderAsset.h"
#include "runtime/runtime.h"
#include "spirv_cross/spirv_cross.hpp"
#include "fileManager/fileManager.h"
#include <filesystem>
#include <memory>

ShaderCompiler::ShaderCompiler() {}

ShaderIncluder::ShaderIncluder()
{
    _search_dirs.push_back(std::filesystem::current_path() / "defaultAssets" / "shaders");
}

shaderc_include_result* ShaderIncluder::GetInclude(
    const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth)
{

    std::cout << "Shader wants: '" << requested_source << "' and is asking from: '" << requesting_source << "'"
              << std::endl;

    auto reqPath = std::filesystem::path(requesting_source).parent_path();

    std::vector<char> file;

    std::cout << "Searching " << _search_dirs.size() << " dirs" << std::endl;
    for(auto dir : _search_dirs) {
        std::cout << "searching " << dir << std::endl;
        auto path = dir / requested_source;
        if(FileManager::readFile(path, file)) {
            std::cout << "Found: " << path << std::endl;
            shaderc_include_result* result = new shaderc_include_result{};
            auto pathStr = path.u8string();
            result->source_name = new char[pathStr.size()];
            std::memcpy((void*)result->source_name, pathStr.data(), pathStr.size());
            result->source_name_length = pathStr.size();

            result->content = new char[file.size()];
            std::memcpy((void*)result->content, file.data(), file.size());
            result->content_length = file.size();
            std::cout << "returning result source_name="
                      << std::string_view(result->source_name, result->source_name_length) << std::endl;
            return result;
        }
    }

    auto errorRes = new shaderc_include_result{};
    errorRes->source_name = "";
    errorRes->source_name_length = 0;
    std::string errMessage = std::format(
        "'{}' requested by '{}' was not found", std::string(requested_source), std::string(requesting_source));
    errorRes->content = new char[errMessage.size()];
    errorRes->content_length = errMessage.size();

    return errorRes;
}

// Handles shaderc_include_result_release_fn callbacks.
void ShaderIncluder::ReleaseInclude(shaderc_include_result* data)
{
    if(data->source_name_length > 0)
        delete data->source_name;
    delete data->content;
    delete data;
}

void ShaderIncluder::addSearchDir(std::filesystem::path dir) { _search_dirs.push_back(dir); }

bool ShaderCompiler::compileShader(
    const std::string& glsl,
    ShaderType type,
    std::vector<uint32_t>& spirv,
    std::unique_ptr<ShaderIncluder> includer,
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
    options.SetIncluder(std::unique_ptr<shaderc::CompileOptions::IncluderInterface>(
        (shaderc::CompileOptions::IncluderInterface*)includer.release()));

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
    const std::string& glsl,
    ShaderType shaderType,
    std::unique_ptr<ShaderIncluder> includer,
    ShaderAttributes& attributes)
{
    // We need to compile without optimization to be able to extract variable names
    std::vector<uint32_t> spirv;
    if(!compileShader(glsl, shaderType, spirv, std::move(includer), false))
        return false;

    spirv_cross::Compiler compiler(std::move(spirv));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    for(auto& uniformBuffer : resources.uniform_buffers) {
        UniformBufferData ub{};
        ub.name = uniformBuffer.name;

        std::cout << "Found UB " << ub.name << std::endl;
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

        std::cout << "Found stage input " << var.name << std::endl;
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
