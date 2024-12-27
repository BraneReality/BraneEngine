//
// Created by eli on 8/25/2022.
//

#ifndef BRANEENGINE_EDITORMATERIALASSET_H
#define BRANEENGINE_EDITORMATERIALASSET_H

#include "../editorAsset.h"

class EditorShaderAsset;

class EditorMaterialAsset : public EditorAsset
{
  public:
    EditorMaterialAsset(const std::filesystem::path& file, BraneProject& project);

    std::vector<std::pair<AssetID, AssetType>> containedAssets() const override;

    Asset* buildAsset(const AssetID& id) const override;

    void initializeProperties(EditorShaderAsset* shaderAsset);

    std::vector<uint8_t> serializeProperties() const;

    void changeProperty(size_t index, Json::Value value, bool finished);
};

#endif // BRANEENGINE_EDITORMATERIALASSET_H
