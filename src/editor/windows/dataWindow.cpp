//
// Created by eli on 5/21/2022.
//

#include "dataWindow.h"
#include "../widgets/assetSelectWidget.h"
#include "../widgets/virtualVariableWidgets.h"
#include "assets/assetManager.h"
#include "assets/types/materialAsset.h"
#include "assets/types/meshAsset.h"
#include "common/ecs/entity.h"
#include "editor/assets/editorAsset.h"
#include "editor/assets/types/editorAssemblyAsset.h"
#include "editor/assets/types/editorMaterialAsset.h"
#include "editor/assets/types/editorShaderAsset.h"
#include "editor/editor.h"
#include "editor/editorEvents.h"
#include "systems/transforms.h"
#include <assets/assembly.h>
#include <assets/assetID.h>
#include <ui/gui.h>

#include "imgui_impl_vulkan.h"
#include "ecs/nativeTypes/meshRenderer.h"
#include "editor/assets/assemblyReloadManager.h"
#include "graphics/graphics.h"
#include "graphics/pointLightComponent.h"
#include "graphics/texture.h"
#include "imgui_stdlib.h"
#include "ui/guiPopup.h"

#include "assets/types/imageAsset.h"
#include "utility/jsonTypeUtilities.h"

DataWindow::DataWindow(GUI& ui, Editor& editor) : EditorWindow(ui, editor)
{
    _name = "Data Inspector";
    ui.addEventListener<FocusAssetEvent>("focus asset", this, [this](const FocusAssetEvent* event) {
        _focusedAsset = event->asset();
        _focusMode = FocusMode::asset;
        _focusedAssetEntity = -1;

        if(_focusedAsset->type() == AssetType::image) {
            _imagePreview = VK_NULL_HANDLE;
            _previewImageAsset = nullptr;
            Runtime::getModule<AssetManager>()
                ->fetchAsset<ImageAsset>(AssetID(_focusedAsset->json()["id"].asString()))
                .then([this](ImageAsset* image) {
                    _previewImageAsset = image;
                    auto* texture = Runtime::getModule<graphics::VulkanRuntime>()->getTexture(image->runtimeID);
                    if(texture)
                        _imagePreview = ImGui_ImplVulkan_AddTexture(
                            texture->sampler(), texture->view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                });
        }
        else if(_focusedAsset->type() == AssetType::material) {
            auto* material = static_cast<EditorMaterialAsset*>(_focusedAsset.get());
            auto frag = _editor.project().getEditorAsset(AssetID(material->json()["fragmentShader"].asString()));
            if(frag)
                material->initializeProperties(static_cast<EditorShaderAsset*>(frag.get()));
        }
    });
    ui.addEventListener<FocusEntityAssetEvent>("focus entity asset", this, [this](const FocusEntityAssetEvent* event) {
        _focusedAssetEntity = event->entity();
    });
    ui.addEventListener<FocusEntityEvent>("focus entity", this, [this](const FocusEntityEvent* event) {
        _focusedEntity = event->id();
        _focusMode = FocusMode::entity;
    });
}

DataWindow::~DataWindow() {}

void DataWindow::displayContent()
{
    switch(_focusMode) {
    case FocusMode::asset:
        displayAssetData();
        break;
    case FocusMode::entity:
        displayEntityData();
        break;
    default:
        ImGui::TextDisabled("No data to display");
    }
}

void DataWindow::displayAssetData()
{
    if(!_focusedAsset)
        return;

    ImGui::PushFont(_ui.fonts()[1]);
    ImGui::Text("%s%s", _focusedAsset->name().c_str(), (_focusedAsset->unsavedChanges()) ? " *" : "");
    ImGui::PopFont();
    ImGui::TextDisabled("%s", _focusedAsset->type().toString().c_str());
#ifdef NDEBUG
    try {
#endif
        switch(_focusedAsset->type().type()) {
        case AssetType::shader:
            ImGui::Text("Source: %s", _focusedAsset->json()["source"].asCString());
            break;
        case AssetType::mesh:
            displayMeshData();
            break;
        case AssetType::assembly:
            displayAssemblyData();
            break;
        case AssetType::material:
            displayMaterialData();
            break;
        case AssetType::image:
            displayImageData();
            break;
        case AssetType::chunk:
            displayChunkData();
            break;
        default:
            ImGui::PushTextWrapPos();
            ImGui::Text(
                "Asset Type %s not implemented yet. If you want to edit %s go to the GitHub and open an issue to put "
                "pressure on me.",
                _focusedAsset->type().toString().c_str(),
                _focusedAsset->name().c_str());
            ImGui::PopTextWrapPos();
        }
#ifdef NDEBUG
    }
    catch(const std::exception& e) {
        ImGui::TextColored({1, 0, 0, 1}, "Error displaying asset: %s", e.what());
    }
#endif
}

void DataWindow::displayChunkData()
{
    ImGui::Text("LODs:");

    ImGui::Indent();
    int lodIndex = 0;
    auto& lods = _focusedAsset->json()["LODs"];
    int removedLOD = -1;
    for(auto& lod : lods) {
        ImGui::PushID(lodIndex);
        ImGui::TextDisabled("%d", lodIndex);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        int dragInt[2] = {lod["min"].asInt(), lod["max"].asInt()};
        if(ImGui::DragInt2("##Range", dragInt)) {
            if(dragInt[0] < 0)
                dragInt[0] = 0;
            if(dragInt[1] < 0)
                dragInt[1] = 0;
            if(dragInt[0] > dragInt[1])
                dragInt[1] = dragInt[0];
            Json::Value newLod = lod;
            newLod["min"] = dragInt[0];
            newLod["max"] = dragInt[1];
            _focusedAsset->json().changeValue("LODs/" + std::to_string(lodIndex), newLod);
        }
        ImGui::SameLine();
        AssetID assembly(lods[lodIndex]["assembly"].asString());
        ImGui::SetNextItemWidth(160);
        if(AssetSelectWidget::draw(assembly, AssetType::assembly)) {
            Json::Value newLod = lod;
            newLod["assembly"] = assembly.string();
            _focusedAsset->json().changeValue("LODs/" + std::to_string(lodIndex), newLod);
        }
        if(assembly.null()) {
            ImGui::SameLine();
            if(ImGui::Button("Create Assembly")) {
                std::filesystem::path lodPath = _focusedAsset->file();
                lodPath.replace_filename(_focusedAsset->name() + "_LOD" + std::to_string(lodIndex) + ".assembly");

                auto* lodAssembly = new EditorAssemblyAsset(lodPath, _editor.project());

                Json::Value newLod = lod;
                newLod["assembly"] = lodAssembly->json()["id"];
                _focusedAsset->json().changeValue("LODs/" + std::to_string(lodIndex), newLod);
                lodAssembly->save();
                delete lodAssembly;
            }
        }
        if(lods.size() > 1) {
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 13);
            if(ImGui::Button("-", {15, 0}))
                removedLOD = lodIndex;
        }

        ImGui::PopID();
        ++lodIndex;
    }
    if(removedLOD != -1) {
        Json::Value newLODList;
        for(Json::ArrayIndex i = 0; i < lods.size(); ++i) {
            if(i != removedLOD)
                newLODList.append(lods[i]);
        }
        _focusedAsset->json().changeValue("LODs", newLODList);
    }

    ImGui::Unindent();
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 13);
    if(ImGui::Button("+", {15, 0})) {
        Json::Value newLODList = _focusedAsset->json()["LODs"];
        Json::Value newLod;
        newLod["min"] = 0;
        newLod["max"] = 0;
        newLod["assembly"] = "null";
        newLODList.append(newLod);
        _focusedAsset->json().changeValue("LODs", newLODList);
    }
}

void DataWindow::displayAssemblyData()
{
    if(_focusedAssetEntity < _focusedAsset->json()["entities"].size()) {
        displayEntityAssetData();
        return;
    }
    if(ImGui::CollapsingHeader("Dependencies")) {
        ImGui::Indent();
        if(ImGui::CollapsingHeader("Materials")) {
            ImGui::Indent();
            int materialIndex = 0;
            std::pair<int, AssetID> changedMaterial = {-1, AssetID()};
            for(auto& cID : _focusedAsset->json()["dependencies"]["materials"]) {
                AssetID materialID(cID.asString());
                ImGui::PushID(materialIndex);
                if(AssetSelectWidget::draw(materialID, AssetType::material))
                    changedMaterial = {materialIndex, std::move(materialID)};
                ImGui::PopID();
                ++materialIndex;
            }
            if(changedMaterial.first >= 0) {
                dynamic_cast<EditorAssemblyAsset*>(_focusedAsset.get())
                    ->changeMaterial(changedMaterial.first, changedMaterial.second);
                _editor.reloadAsset(_focusedAsset);
            }
            ImGui::Unindent();
        }
        if(ImGui::CollapsingHeader("Meshes")) {
            ImGui::Indent();
            for(auto& mID : _focusedAsset->json()["dependencies"]["meshes"]) {
                ImGui::Selectable(mID.asCString());
            }
            ImGui::Unindent();
        }
        ImGui::Unindent();
    }
    ImGui::Text("Entities: %u", _focusedAsset->json()["entities"].size());
    if(ImGui::IsItemHovered())
        ImGui::SetTooltip("Edit in entities window");
}

void DataWindow::displayMeshData() {}

class AddAssetComponentPopup : public GUIPopup {
    AssetSearchWidget _search;
    std::shared_ptr<EditorAssemblyAsset> _focusedAsset;
    Json::ArrayIndex _focusedEntity;

    void drawBody() override
    {
        if(_search.draw()) {
            auto asset = _focusedAsset;
            auto entity = _focusedEntity;
            Runtime::getModule<AssetManager>()
                ->fetchAsset<ComponentAsset>(_search.currentSelected())
                .then([this, asset, entity](ComponentAsset* component) {
                    auto* compDef = Runtime::getModule<EntityManager>()->components().getComponentDef(component);
                    VirtualComponent newComp(compDef);
                    _focusedAsset->addEntityComponent(_focusedEntity, EditorAssemblyAsset::componentToJson(newComp));
                });
            ImGui::CloseCurrentPopup();
        }
    }

  public:
    AddAssetComponentPopup(std::shared_ptr<EditorAssemblyAsset> focusedAsset, Json::ArrayIndex focusedEntity)
        : _focusedAsset(std::move(focusedAsset)), _focusedEntity(focusedEntity), _search(AssetType::component),
          GUIPopup("add component") {};
};

void DataWindow::displayEntityAssetData()
{
    auto* assembly = dynamic_cast<EditorAssemblyAsset*>(_focusedAsset.get());
    auto& entityAsset = _focusedAsset->json()["entities"][(Json::ArrayIndex)_focusedAssetEntity];
    ImGui::PushFont(_ui.fonts()[1]);
    std::string entityName = entityAsset["name"].asString();
    ImGui::InputText("##EntityName", &entityName);
    if(ImGui::IsItemDeactivatedAfterEdit())
        _focusedAsset->json().changeValue("entities/" + std::to_string(_focusedAssetEntity) + "/name", entityName);

    ImGui::PopFont();
    ImGui::TextDisabled("Index: %u", _focusedAssetEntity);
    ImGui::Separator();
    int deleteComponent = -1;
    for(Json::ArrayIndex i = 0; i < entityAsset["components"].size(); ++i) {
        auto& component = entityAsset["components"][i];
        if(component["name"].empty())
            continue;
        ImGui::PushID(component["name"].asCString());
        bool displaying = ImGui::CollapsingHeader(component["name"].asCString(), ImGuiTreeNodeFlags_DefaultOpen);
        if(ImGui::BeginPopupContextItem("comp actions")) {
            bool transform = component["name"] == "transform";
            if(!transform) {
                if(ImGui::Selectable(ICON_FA_TRASH "delete"))
                    deleteComponent = i;
            }
            ImGui::EndPopup();
        }
        /*if(ImGui::BeginDragDropSource())
        {
            ImGui::TextDisabled("%s", component["name"].asCString());
            DraggedComponent dragData{(Assembly*)_focusedAsset->asset(), _focusedAssetEntity, i};
            ImGui::SetDragDropPayload("component", &dragData, sizeof(DraggedComponent));
            ImGui::EndDragDropSource();
        }
        if(ImGui::BeginDragDropTarget())
        {
            if(const ImGuiPayload* p = ImGui::AcceptDragDropPayload("component"))
            {
                auto dc = static_cast<DraggedComponent*>(p->Data);
                auto& srcComponents = dc->asset->entities[dc->entity].components;
                VirtualComponent source = srcComponents[dc->componentIndex];
                srcComponents.erase(srcComponents.begin() + dc->componentIndex);
                auto& destComponents = entityAsset.components;
                // The min is to account for if we're trying to move the component to the end, and just removed it from
        the same list. destComponents.insert(destComponents.begin() + std::min<uint32_t>(destComponents.size(), i),
        source);
            }
            ImGui::EndDragDropTarget();
        }*/
        if(displaying) {
            ImGui::Indent();
            if(component["name"].asString() == MeshRendererComponent::def()->name) {
                ImGui::Text("Mesh Index: %d", component["members"][0]["value"].asInt());
                for(Json::ArrayIndex mat = 0; mat < component["members"][1]["value"].size(); ++mat) {
                    ImGui::PushID(mat);
                    Json::ArrayIndex matIndex = component["members"][1]["value"][mat].asUInt();
                    ImGui::Text("Material %u (Assembly index %u)", mat, matIndex);
                    ImGui::SameLine();
                    AssetID matID(_focusedAsset->json()["dependencies"]["materials"].get(matIndex, "null").asString());
                    if(AssetSelectWidget::draw(matID, AssetType::material)) {
                        dynamic_cast<EditorAssemblyAsset*>(_focusedAsset.get())->changeMaterial(matIndex, matID);
                        _editor.reloadAsset(_focusedAsset);
                    }
                    ImGui::PopID();
                }
            }
            else if(component["name"].asString() == PointLightComponent::def()->name) {
                auto lightValue = fromJson<glm::vec4>(component["members"][0]["value"]);
                ImGui::ColorEdit3("Tint", (float*)&lightValue);
                bool change = ImGui::IsItemEdited();
                bool endChange = ImGui::IsItemDeactivatedAfterEdit();
                ImGui::DragFloat("Strength", &lightValue[3], 0.005);
                change = change || ImGui::IsItemEdited();
                endChange = endChange || ImGui::IsItemDeactivatedAfterEdit();
                if(change || endChange) {
                    Json::Value data = component;
                    data["members"][0]["value"] = toJson(lightValue);
                    assembly->updateEntityComponent(_focusedAssetEntity, i, data, endChange);
                }
            }
            else {
                Json::Value data = component;
                auto res = VirtualVariableWidgets::displayAssetComponentData(data, _focusedAsset->json().data());
                if(res != UiChangeType::none) {
                    assembly->updateEntityComponent(_focusedAssetEntity, i, data, res != UiChangeType::finished);
                    if(res == UiChangeType::finished) {
                        _editor.reloadAsset(_focusedAsset);
                    }
                }
            }
            ImGui::Unindent();
        }
        ImGui::PopID();
    }
    if(ImGui::Button("Add Component", {ImGui::GetContentRegionAvail().x, 0})) {
        _ui.openPopup(std::make_unique<AddAssetComponentPopup>(
            std::dynamic_pointer_cast<EditorAssemblyAsset>(_focusedAsset), _focusedAssetEntity));
    }

    if(deleteComponent != -1)
        assembly->removeEntityComponent(_focusedAssetEntity, deleteComponent);
}

void DataWindow::displayEntityData()
{

    ImGui::Text("Entity ID: %u", _focusedEntity.id);
    ImGui::TextDisabled("Version: %u", _focusedEntity.version);
    ImGui::Separator();
    auto* em = Runtime::getModule<EntityManager>();
    if(!em->entityExists(_focusedEntity)) {
        ImGui::TextColored({1, 0, 0, 1}, "Entity Destroyed!");
        return;
    }

    auto& components = em->getEntityArchetype(_focusedEntity)->components();
    for(auto cid : components) {
        if((int)VirtualVariableWidgets::displayVirtualComponentData(em->getComponent(_focusedEntity, cid)) > 0)
            em->markComponentChanged(_focusedEntity, cid);
        ImGui::Separator();
    }
}

void DataWindow::displayMaterialData()
{
    auto* material = static_cast<EditorMaterialAsset*>(_focusedAsset.get());
    AssetID vertexID(material->json()["vertexShader"].asString());
    if(AssetSelectWidget::draw(vertexID, AssetType::shader)) {
        material->json().changeValue("vertexShader", vertexID.string());
        _editor.reloadAsset(_focusedAsset);
    };
    ImGui::SameLine();
    ImGui::Text("Vertex Shader");
    AssetID fragmentID(material->json()["fragmentShader"].asString());
    if(AssetSelectWidget::draw(fragmentID, AssetType::shader)) {
        material->json().changeValue("fragmentShader", fragmentID.string());
        auto frag = _editor.project().getEditorAsset(fragmentID);
        material->initializeProperties(static_cast<EditorShaderAsset*>(frag.get()));
        _editor.reloadAsset(_focusedAsset);
    }

    ImGui::SameLine();
    ImGui::Text("Fragment Shader");
    ImGui::Spacing();

    uint32_t samplerIndex = 0;
    if(!vertexID.null()) {
        ImGui::Text("Vertex Shader:");
        ImGui::Indent();
        const auto shader = _editor.project().getEditorAsset(vertexID);
        if(shader)
            displayShaderAttributes(shader.get(), material);
        ImGui::Unindent();
        ImGui::Separator();
    }
    if(!fragmentID.null()) {
        ImGui::Text("Fragment Shader:");
        ImGui::Indent();
        const auto shader = _editor.project().getEditorAsset(fragmentID);
        if(shader)
            displayShaderAttributes(shader.get(), material);
        ImGui::Unindent();
        ImGui::Separator();
        ImGui::Text("Properties");
        ImGui::Indent();
        size_t propIndex = 0;
        for(auto& prop : material->json()["properties"]) {
            auto& type = prop["type"];
            if(type == "bool") {
                bool value = prop["value"].asBool();
                ImGui::Checkbox(prop["name"].asCString(), &value);
                if(ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit())
                    material->changeProperty(propIndex, value, ImGui::IsItemDeactivatedAfterEdit());
            }
            else if(type == "float") {
                float value = prop["value"].asFloat();
                ImGui::DragFloat(prop["name"].asCString(), &value, 0.05f);
                if(ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit())
                    material->changeProperty(propIndex, value, ImGui::IsItemDeactivatedAfterEdit());
            }
            else if(type == "int") {
                int value = prop["value"].asInt();
                ImGui::DragInt(prop["name"].asCString(), &value);
                if(ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit())
                    material->changeProperty(propIndex, value, ImGui::IsItemDeactivatedAfterEdit());
            }
            else if(type == "vec2") {
                auto value = fromJson<glm::vec2>(prop["value"]);
                ImGui::DragFloat2(prop["name"].asCString(), (float*)&value, 0.05f);
                if(ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit())
                    material->changeProperty(propIndex, toJson(value), ImGui::IsItemDeactivatedAfterEdit());
            }
            else if(type == "vec3") {
                glm::vec4 value = {fromJson<glm::vec3>(prop["value"]), 1};
                ImGui::DragFloat3(prop["name"].asCString(), (float*)&value, 0.05f);
                bool edited = ImGui::IsItemEdited();
                bool finished = ImGui::IsItemDeactivatedAfterEdit();
                ImGui::SameLine();
                if(ImGui::ColorButton("##ColorButton", *(ImVec4*)&value)) {
                    ImGui::OpenPopup("picker");
                }
                if(ImGui::BeginPopup("picker")) {
                    ImGui::ColorPicker3("##picker", (float*)&value);
                    edited |= ImGui::IsItemEdited();
                    finished |= ImGui::IsItemDeactivatedAfterEdit();
                    ImGui::EndPopup();
                }
                if(edited || finished)
                    material->changeProperty(propIndex, toJson(glm::vec3{value.x, value.y, value.z}), finished);
            }
            else if(type == "vec4") {
                glm::vec4 value = fromJson<glm::vec4>(prop["value"]);
                ImGui::DragFloat4(prop["name"].asCString(), (float*)&value, 0.05f);
                bool edited = ImGui::IsItemEdited();
                bool finished = ImGui::IsItemDeactivatedAfterEdit();
                ImGui::SameLine();
                if(ImGui::ColorButton("##ColorButton", *(ImVec4*)&value)) {
                    ImGui::OpenPopup("picker");
                }
                if(ImGui::BeginPopup("picker")) {
                    ImGui::ColorPicker4("##picker", (float*)&value);
                    edited |= ImGui::IsItemEdited();
                    finished |= ImGui::IsItemDeactivatedAfterEdit();
                    ImGui::EndPopup();
                }
                if(edited || finished)
                    material->changeProperty(propIndex, toJson(value), finished);
            }

            propIndex++;
        }
        ImGui::Unindent();
        ImGui::Separator();
        ImGui::Text("Textures");
        ImGui::Indent();
        for(auto& sampler : shader->json()["attributes"]["samplers"]) {
            std::string sampName = sampler["name"].asString();
            AssetID imageID;
            if(material->json()["textures"].isMember(sampName))
                imageID = material->json()["textures"][sampName]["id"].asString();
            ImGui::PushID(sampName.c_str());
            if(AssetSelectWidget::draw(imageID, AssetType::image)) {
                Json::Value samplerJson;
                samplerJson["id"] = imageID.string();
                samplerJson["binding"] = sampler["binding"];
                material->json().changeValue("textures/" + sampName, samplerJson);
                _editor.reloadAsset(_focusedAsset);
            }
            ImGui::SameLine();
            ImGui::Text("%s", sampName.c_str());
            ImGui::PopID();
        }
        ImGui::Unindent();
    }
}

void DataWindow::displayShaderAttributes(EditorAsset* asset, EditorMaterialAsset* material)
{
    ImGui::Text("inputs:");
    ImGui::Indent();
    const auto& attributes = asset->json()["attributes"];
    for(auto& input : attributes["inputs"])
        ImGui::Text("%s %s", input["layout"].asCString(), input["name"].asCString());
    ImGui::Unindent();
    ImGui::Text("outputs:");
    ImGui::Indent();
    for(auto& output : attributes["outputs"])
        ImGui::Text("%s %s", output["layout"].asCString(), output["name"].asCString());
    ImGui::Unindent();
    if(attributes.isMember("uniforms")) {
        ImGui::Text("uniforms:");
        ImGui::Indent();
        for(auto& uniformName : attributes["uniforms"].getMemberNames()) {
            auto& uniform = attributes["uniforms"][uniformName];
            ImGui::Text("name: %s", uniformName.c_str());
            ImGui::Text("members:");
            ImGui::Indent();
            for(auto& member : uniform["members"])
                ImGui::Text("%s %s", member["layout"].asCString(), member["name"].asCString());
            if(uniform[""])

                ImGui::Unindent();
        }
        ImGui::Unindent();
    }
    if(attributes.isMember("buffers")) {
        ImGui::Text("buffers:");
        ImGui::Indent();
        for(auto& uniformName : attributes["buffers"].getMemberNames()) {
            auto& uniform = attributes["buffers"][uniformName];
            ImGui::Text("name: %s", uniformName.c_str());
            ImGui::Text("members:");
            ImGui::Indent();
            for(auto& member : uniform["members"])
                ImGui::Text("%s %s", member["layout"].asCString(), member["name"].asCString());
            ImGui::Unindent();
        }
        ImGui::Unindent();
    }
}

void DataWindow::displayImageData()
{
    const char* imageTypes[3] = {"color", "normal map", "linear"};
    auto imageType = _focusedAsset->json()["imageType"].asUInt();
    if(ImGui::BeginCombo("ImageType", imageTypes[imageType])) {
        if(ImGui::Selectable("color"))
            _focusedAsset->json().changeValue("imageType", 0);
        if(ImGui::Selectable("normal map"))
            _focusedAsset->json().changeValue("imageType", 1);
        if(ImGui::Selectable("linear"))
            _focusedAsset->json().changeValue("imageType", 2);
        ImGui::EndCombo();
    }
    if(_imagePreview) {
        float width = ImGui::GetContentRegionMax().x;
        float height = width / (float)_previewImageAsset->size.x * _previewImageAsset->size.y;
        ImGui::Image((void*)_imagePreview, {width, height});
    }
    else if(_previewImageAsset) {
        ImGui::TextDisabled("Loading image preview...");
        auto* texture = Runtime::getModule<graphics::VulkanRuntime>()->getTexture(_previewImageAsset->runtimeID);
        if(texture)
            _imagePreview = ImGui_ImplVulkan_AddTexture(
                texture->sampler(), texture->view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}
