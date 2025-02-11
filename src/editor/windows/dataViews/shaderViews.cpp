#include "shaderViews.h"
#include "assets/assetManager.h"
#include "imgui.h"
#include "imgui_stdlib.h"

ShaderSourceView::ShaderSourceView(Shared<ShaderAssetSource> source) : _source(source)
{
    _editor = Runtime::getModule<Editor>();
}

Result<void> ShaderSourceView::draw()
{
    ImGui::Text("inputs:");
    ImGui::Indent();
    for(auto& input : _source->attributes.inputVariables)
        ImGui::Text("%s %s", ShaderVariableData::layoutNames.toString(input.layout()).c_str(), input.name.c_str());
    ImGui::Unindent();
    ImGui::Text("outputs:");
    ImGui::Indent();
    for(auto& output : _source->attributes.outputVariables)
        ImGui::Text("%s %s", ShaderVariableData::layoutNames.toString(output.layout()).c_str(), output.name.c_str());
    ImGui::Unindent();
    if(!_source->attributes.uniforms.empty())
    {
        ImGui::Text("uniforms:");
        ImGui::Indent();
        for(auto& uniform : _source->attributes.uniforms)
        {
            ImGui::Text("name: %s", uniform.name.c_str());
            ImGui::Text("members:");
            ImGui::Indent();
            for(auto& member : uniform.members)
                ImGui::Text(
                    "%s %s", ShaderVariableData::layoutNames.toString(member.layout()).c_str(), member.name.c_str());
            ImGui::Unindent();
        }
        ImGui::Unindent();
    }
    if(!_source->attributes.buffers.empty())
    {
        ImGui::Text("buffers:");
        ImGui::Indent();
        for(auto& uniform : _source->attributes.buffers)
        {
            ImGui::Text("name: %s", uniform.name.c_str());
            ImGui::Text("members:");
            ImGui::Indent();
            for(auto& member : uniform.members)
                ImGui::Text(
                    "%s %s", ShaderVariableData::layoutNames.toString(member.layout()).c_str(), member.name.c_str());
            ImGui::Unindent();
        }
        ImGui::Unindent();
    }
    return Ok<void>();
}
