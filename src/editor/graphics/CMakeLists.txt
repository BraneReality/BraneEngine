
find_package(unofficial-shaderc CONFIG REQUIRED)

find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)
find_package(spirv_cross_hlsl CONFIG REQUIRED)
find_package(spirv_cross_msl CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_c CONFIG REQUIRED)

find_package(glslang CONFIG REQUIRED)

add_library(shaderCompiler STATIC shaderCompiler.cpp)
target_link_libraries(shaderCompiler PUBLIC  
    unofficial::shaderc::shaderc 
    unofficial::shaderc::shaderc_util
    spirv-cross-c
    spirv-cross-core
    spirv-cross-cpp 
    spirv-cross-glsl
    glslang::glslang
    glslang::SPIRV 
    glslang::SPVRemapper)
