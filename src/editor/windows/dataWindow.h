//
// Created by eli on 5/21/2022.
//

#ifndef BRANEENGINE_DATAWINDOW_H
#define BRANEENGINE_DATAWINDOW_H

#include <memory>
#include "dataViews/dataView.h"
#include "editor/state/versioning.h"
#include "editorWindow.h"
#include "utility/event.h"
#include "utility/option.h"
#include "utility/shared.h"
#include "vulkan/vulkan.hpp"

#include "ecs/entityID.h"

class EditorAsset;

class Assembly;

class ImageAsset;

class EditorMaterialAsset;

class DataWindow : public EditorWindow
{
    Option<Event<Option<Shared<EditorAsset>>, EditorActionType>::Handle> _onFocusHandle;

    enum class FocusMode
    {
        asset,
        entity
    };
    FocusMode _focusMode;
    Option<Shared<EditorAsset>> _focusedAsset;
    uint32_t _focusedAssetEntity = 0;

    std::vector<Shared<DataView>> _views;

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


    void displayImageData();

    void displayShaderAttributes(EditorAsset* asset, EditorMaterialAsset* material);

    void displayContent() override;

  public:
    DataWindow(GUI& ui, Editor& editor);

    ~DataWindow();
};

#endif // BRANEENGINE_DATAWINDOW_H
