//
// Created by eli on 9/26/2022.
//

#ifndef BRANEENGINE_EDITORCHUNKASSET_H
#define BRANEENGINE_EDITORCHUNKASSET_H

#include "../editorAsset.h"

class EditorChunkAsset : public EditorAsset
{
    Asset* buildAssembly() const;

    Asset* buildMesh(const AssetID& id) const;

  public:
    EditorChunkAsset(const std::filesystem::path& file, BraneProject& project);

    std::vector<std::pair<AssetID, AssetType>> containedAssets() const override;

    Result<std::unique_ptr<Asset>> buildAsset(const AssetID& id) const override;
};

#endif // BRANEENGINE_EDITORCHUNKASSET_H
