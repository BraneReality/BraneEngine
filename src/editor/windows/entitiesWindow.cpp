//
// Created by eli on 5/21/2022.
//

#include "entitiesWindow.h"
#include <imgui.h>
#include "assets/assembly.h"
#include "assets/assetManager.h"
#include "ecs/entity.h"
#include "editor/assets/assemblyReloadManager.h"
#include "editor/assets/editorAsset.h"
#include "editor/assets/types/editorAssemblyAsset.h"
#include "editor/editor.h"
#include "editor/editorEvents.h"
#include "systems/transforms.h"
#include "ui/gui.h"

EntitiesWindow::EntitiesWindow(GUI& ui, Editor& editor) : EditorWindow(ui, editor)
{
    _name = "Entities";
    _em = Runtime::getModule<EntityManager>();
    ui.addEventListener<FocusAssetEvent>("focus asset", this, [this](const FocusAssetEvent* event) {
        _selected = -1;
        _asset = event->asset();
    });
    ui.addEventListener<FocusEntityAssetEvent>(
        "focus entity asset", this, [this](const FocusEntityAssetEvent* event) { _selected = event->entity(); });
}

void EntitiesWindow::displayContent()
{
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 13);
    if(_asset && _asset->type() == AssetType::assembly)
    {
        _asset->data().beginMultiChange();
        displayAssetEntities(_asset->data()["rootEntity"].asUInt());
        _asset->data().endMultiChange();
    }
    ImGui::PopStyleVar();
    ImGui::Spacing();
}

void EntitiesWindow::displayAssetEntities(uint32_t index, bool isLastChild)
{
    auto* assembly = dynamic_cast<EditorAssemblyAsset*>(_asset.get());
    auto& entity = _asset->data()["entities"][index];
    const bool hasChildren = entity.isMember("children") && !entity["children"].empty();
    const bool isRoot = _asset->data()["rootEntity"].asUInt() == index;
    const float reorderHeight = 4;
    const float indentSpacing = ImGui::GetStyle().IndentSpacing;
    const float reorderWidth = ImGui::GetContentRegionMax().x - ImGui::GetCursorPosX() - indentSpacing - 5;

    ImGui::PushID(index);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
    if(!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf;
    if(index == _selected)
        flags |= ImGuiTreeNodeFlags_Selected;
    std::string name;
    if(entity.isMember("name"))
        name = entity["name"].asString();
    else
        name = "Unnamed " + std::to_string(index);

    bool nodeOpen = ImGui::TreeNodeEx(name.c_str(), flags);
    if(ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        _ui.sendEvent(std::make_unique<FocusEntityAssetEvent>(index));
        _selected = index;
    }

    if(ImGui::BeginPopupContextItem())
    {
        if(ImGui::Selectable(ICON_FA_CIRCLE_PLUS " Create Entity"))
            assembly->createEntity(index);
        if(!isRoot && ImGui::Selectable(ICON_FA_TRASH_CAN " Delete Entity"))
            assembly->deleteEntity(index);
        ImGui::EndPopup();
    }

    if(!isRoot && ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("assetEntity", &index, sizeof(uint32_t));
        ImGui::Text("%s", name.c_str());
        ImGui::EndDragDropSource();
    }

    ImRect dropRectMain;
    dropRectMain.Min = {ImGui::GetCursorPosX() + indentSpacing - 2, ImGui::GetItemRectMin().y};
    if(!nodeOpen)
        dropRectMain.Min.x += indentSpacing;
    dropRectMain.Max = {dropRectMain.Min.x + reorderWidth, ImGui::GetItemRectMax().y};
    if(ImGui::BeginDragDropTargetCustom(dropRectMain, index * 3 + 1))
    {
        if(const ImGuiPayload* droppedEntityPayload = ImGui::AcceptDragDropPayload("assetEntity"))
        {
            uint32_t droppedIndex = *(uint32_t*)droppedEntityPayload->Data;
            assembly->parentEntity(droppedIndex, index, 0);
        }
        ImGui::EndDragDropTarget();
    }

    if(!isRoot)
    {
        ImRect dropAboveRect;
        dropAboveRect.Min = {dropRectMain.Min.x, dropRectMain.Min.y - reorderHeight};
        dropAboveRect.Max = {dropAboveRect.Min.x + reorderWidth, dropRectMain.Min.y};
        if(ImGui::BeginDragDropTargetCustom(dropAboveRect, index * 3))
        {
            if(const ImGuiPayload* droppedEntityPayload = ImGui::AcceptDragDropPayload("assetEntity"))
            {
                uint32_t droppedIndex = *(uint32_t*)droppedEntityPayload->Data;
                Json::ArrayIndex parentIndex = entity["parent"].asUInt();
                Json::ArrayIndex currentIndex = 0;
                for(auto& child : _asset->data()["entities"][parentIndex]["children"])
                {
                    if(child.asUInt() == index)
                        break;
                    ++currentIndex;
                }
                assembly->parentEntity(droppedIndex, parentIndex, currentIndex);
            }
            ImGui::EndDragDropTarget();
        }

        if(isLastChild)
        {
            ImRect dropBelowRect;
            dropBelowRect.Min = {dropRectMain.Min.x, dropRectMain.Max.y};
            dropBelowRect.Max = {dropBelowRect.Min.x + reorderWidth, dropBelowRect.Min.y + reorderHeight};
            if(ImGui::BeginDragDropTargetCustom(dropBelowRect, index * 3 + 2))
            {
                if(const ImGuiPayload* droppedEntityPayload = ImGui::AcceptDragDropPayload("assetEntity"))
                {
                    uint32_t droppedIndex = *(uint32_t*)droppedEntityPayload->Data;
                    Json::ArrayIndex parentIndex = entity["parent"].asUInt();
                    Json::ArrayIndex currentIndex = 0;
                    for(auto& child : _asset->data()["entities"][parentIndex]["children"])
                    {
                        if(child.asUInt() == index)
                            break;
                        ++currentIndex;
                    }
                    assembly->parentEntity(droppedIndex, parentIndex, currentIndex + 1);
                }
                ImGui::EndDragDropTarget();
            }
        }
    }

    if(nodeOpen)
    {
        if(hasChildren)
        {
            size_t childIndex = 0;
            for(auto& child : entity["children"])
                displayAssetEntities(child.asUInt(), ++childIndex == entity["children"].size());
        }

        ImGui::TreePop();
    }
    ImGui::PopID();
}
