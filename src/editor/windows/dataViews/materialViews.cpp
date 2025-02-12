#include "materialViews.h"
#include "assets/assetManager.h"
#include "editor/widgets/assetSelectWidget.h"
#include "imgui.h"
#include "imgui_stdlib.h"

MaterialSourceView::MaterialSourceView(Shared<MaterialAssetSource> source) : _source(source)
{
    _editor = Runtime::getModule<Editor>();
    source->validateProperties();
}

Result<void> MaterialSourceView::draw()
{
    auto& am = _editor->actionManager();
    AssetID vertexShader = *_source->vertexShader->value();
    if(AssetSelectWidget::draw(vertexShader, AssetType::shader))
    {
        am.executeAction(_source->vertexShader->set(vertexShader));
        _source->validateProperties();
        //_editor->reloadAsset();
    };
    ImGui::SameLine();
    ImGui::Text("Vertex Shader");
    AssetID fragmentShader = *_source->fragmentShader->value();
    if(AssetSelectWidget::draw(fragmentShader, AssetType::shader))
    {
        am.executeAction(_source->fragmentShader->set(fragmentShader));
        _source->validateProperties();
        //_editor.reloadAsset(_focusedAsset);
    }
    ImGui::SameLine();
    ImGui::Text("Fragment Shader");

    ImGui::Spacing();

    ImGui::Text("Properties");
    ImGui::Indent();
    for(auto& trackedProp : *_source->properties->values())
    {
        MaterialAssetSource::PropVar prop = *trackedProp->value();
        // bool, int, float, glm::vec2, glm::vec3, glm::vec4
        MATCHV(prop.value, [&](bool cv) {
            ImGui::Checkbox(prop.name->c_str(), &cv);
            if(ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit())
            {
                prop.value = cv;
                am.executeAction(trackedProp->set(prop));
            }
        }, [&](int cv) {
            ImGui::DragInt(prop.name->c_str(), &cv);
            if(ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit())
            {
                prop.value = cv;
                am.executeAction(trackedProp->set(prop));
            }
        }, [&](float cv) {
            ImGui::DragFloat(prop.name->c_str(), &cv, 0.05f);
            if(ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit())
            {
                prop.value = cv;
                am.executeAction(trackedProp->set(prop));
            }
        }, [&](glm::vec2 cv) {
            ImGui::DragFloat2(prop.name->c_str(), (float*)&cv, 0.05f);
            if(ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit())
            {
                prop.value = cv;
                am.executeAction(trackedProp->set(prop));
            }
        }, [&](glm::vec3 cv) {
            ImGui::DragFloat3(prop.name->c_str(), (float*)&cv, 0.05f);
            bool edited = ImGui::IsItemEdited();
            bool finished = ImGui::IsItemDeactivatedAfterEdit();
            ImGui::SameLine();
            if(ImGui::ColorButton("##ColorButton", *(ImVec4*)&cv))
            {
                ImGui::OpenPopup("picker");
            }
            if(ImGui::BeginPopup("picker"))
            {
                ImGui::ColorPicker3("##picker", (float*)&cv);
                edited |= ImGui::IsItemEdited();
                finished |= ImGui::IsItemDeactivatedAfterEdit();
                ImGui::EndPopup();
            }
            if(edited || finished)
            {
                prop.value = cv;
                am.executeAction(trackedProp->set(prop));
            }
        }, [&](glm::vec4 cv) {
            ImGui::DragFloat4(prop.name->c_str(), (float*)&cv, 0.05f);
            bool edited = ImGui::IsItemEdited();
            bool finished = ImGui::IsItemDeactivatedAfterEdit();
            ImGui::SameLine();
            if(ImGui::ColorButton("##ColorButton", *(ImVec4*)&cv))
            {
                ImGui::OpenPopup("picker");
            }
            if(ImGui::BeginPopup("picker"))
            {
                ImGui::ColorPicker4("##picker", (float*)&cv);
                edited |= ImGui::IsItemEdited();
                finished |= ImGui::IsItemDeactivatedAfterEdit();
                ImGui::EndPopup();
            }
            if(edited || finished)
            {
                prop.value = cv;
                am.executeAction(trackedProp->set(prop));
            }
        });
    }

    ImGui::Unindent();
    ImGui::Separator();
    ImGui::Text("Textures");
    ImGui::Indent();
    for(auto& binding : *_source->textureBindings->values())
    {
        auto b = *binding->value();
        ImGui::PushID(b.name->c_str());
        if(AssetSelectWidget::draw(b.id, AssetType::image))
        {
            Json::Value samplerJson;
            am.executeAction(binding->set(b));
            //_editor.reloadAsset(_focusedAsset);
        }
        ImGui::SameLine();
        ImGui::Text("%s", b.name->c_str());
        ImGui::PopID();
    }
    ImGui::Unindent();
    return Ok<void>();
}
