////
// Created by eli on 8/31/2022.
//

#ifndef BRANEENGINE_SHADERCOMPILER_H
#define BRANEENGINE_SHADERCOMPILER_H

#include "assets/types/shaderAsset.h"
#include "libshaderc_util/file_finder.h"
#include <shaderc/shaderc.hpp>

class ShaderAsset;

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

    static shaderc_util::FileFinder defaultFinder();

    bool compileShader(
        const std::string& glsl,
        ShaderType type,
        std::vector<uint32_t>& spirv,
        shaderc_util::FileFinder& fileFinder,
        bool optimize = true);

    bool extractAttributes(
        const std::string& glsl, ShaderType type, shaderc_util::FileFinder& fileFinder, ShaderAttributes& attributes);
};

#endif // BRANEENGINE_SHADERCOMPILER_H
