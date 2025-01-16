//
// Created by eli on 5/21/2022.
//

#ifndef BRANEENGINE_DATAWINDOW_H
#define BRANEENGINE_DATAWINDOW_H

#include <memory>
#include "editorWindow.h"
#include "vulkan/vulkan.hpp"

#include "ecs/entityID.h"

class EditorAsset;

class Assembly;

class ImageAsset;

class EditorMaterialAsset;

class DataWindow : public EditorWindow
{
    enum class FocusMode
    {
        asset,
        entity
    };
    FocusMode _focusMode;
    std::shared_ptr<EditorAsset> _focusedAsset;
    uint32_t _focusedAssetEntity = 0;

    struct DraggedComponent
    {
        Assembly* asset;
        size_t entity;
        size_t componentIndex;
    };

    EntityID _focusedEntity;

    void displayAssetData();

    void displayEntityData();

    void displayEntityAssetData();

    void displayChunkData();

    void displayAssemblyData();

    void displayMeshData();

    void displayMaterialData();

    std::string _scriptText;
    void displayScriptData();

    ImageAsset* _previewImageAsset = nullptr;
    VkDescriptorSet _imagePreview = VK_NULL_HANDLE;

    void displayImageData();

    void displayShaderAttributes(EditorAsset* asset, EditorMaterialAsset* material);

    void displayContent() override;

  public:
    DataWindow(GUI& ui, Editor& editor);

    ~DataWindow();
};

#endif // BRANEENGINE_DATAWINDOW_H
