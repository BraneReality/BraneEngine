////
// Created by eli on 8/31/2022.
//

#ifndef BRANEENGINE_SHADERCOMPILER_H
#define BRANEENGINE_SHADERCOMPILER_H

#include "assets/types/shaderAsset.h"
#include <shaderc/shaderc.hpp>
#include <filesystem>

class ShaderAsset;

class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
    std::vector<std::filesystem::path> _search_dirs;

  public:
    ShaderIncluder();

    shaderc_include_result* GetInclude(
        const char* requested_source,
        shaderc_include_type type,
        const char* requesting_source,
        size_t include_depth) override;

    // Handles shaderc_include_result_release_fn callbacks.
    void ReleaseInclude(shaderc_include_result* data) override;

    ~ShaderIncluder() = default;

    void addSearchDir(std::filesystem::path dir);
};

class ShaderCompiler {
  public:
    struct ShaderAttributes {
        std::vector<UniformBufferData> uniforms;
        std::vector<UniformBufferData> buffers;
        std::vector<ShaderVariableData> samplers;
        std::vector<ShaderVariableData> inputVariables;
        std::vector<ShaderVariableData> outputVariables;
    };

    ShaderCompiler();

    bool compileShader(
        const std::string& glsl,
        ShaderType type,
        std::vector<uint32_t>& spirv,
        std::unique_ptr<ShaderIncluder> includer,
        bool optimize = true);

    bool extractAttributes(
        const std::string& glsl,
        ShaderType type,
        std::unique_ptr<ShaderIncluder> includer,
        ShaderAttributes& attributes);
};

#endif // BRANEENGINE_SHADERCOMPILER_H
