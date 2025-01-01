//
// Created by eli on 8/20/2022.
//

#ifndef BRANEENGINE_EDITORASSEMBLYASSET_H
#define BRANEENGINE_EDITORASSEMBLYASSET_H

#include "../editorAsset.h"
#include "ecs/component.h"

class EditorAssemblyAsset : public EditorAsset
{
    Asset* buildAssembly() const;

    Asset* buildMesh(const AssetID& id) const;

    static Json::Value newEntity(uint32_t parent);

    Json::Value _componentBefore = Json::nullValue;

  public:
    EditorAssemblyAsset(const std::filesystem::path& file, BraneProject& project);

    std::vector<std::pair<AssetID, AssetType>> containedAssets() const override;

    Asset* buildAsset(const AssetID& id) const override;

    void linkToGLTF(const std::filesystem::path& file);

    void createEntity(uint32_t parent);

    void deleteEntity(uint32_t entity);

    void parentEntity(uint32_t entity, uint32_t parent, uint32_t index);

    void updateEntityComponent(uint32_t entity, VirtualComponentView component, bool continuous = false);

    void updateEntityComponent(uint32_t entity, uint32_t component, Json::Value value, bool continuous = false);

    void addEntityComponent(uint32_t entity, Json::Value component);

    void removeEntityComponent(uint32_t entity, uint32_t component);

    void changeMaterial(uint32_t matIndex, const AssetID& materialID);

    static Json::Value componentToJson(VirtualComponentView component);

    static Result<VirtualComponent> jsonToComponent(Json::Value component);
};

#endif // BRANEENGINE_EDITORASSEMBLYASSET_H
