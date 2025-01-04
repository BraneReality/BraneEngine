//
// Created by eli on 8/19/2022.
//

#ifndef BRANEENGINE_EDITORSCRIPTASSET_H
#define BRANEENGINE_EDITORSCRIPTASSET_H

#include "../editorAsset.h"
#include "assets/types/scriptAsset.h"

class EditorScriptAsset : public EditorAsset
{
  public:
    EditorScriptAsset(const std::filesystem::path& file, BraneProject& project);

    std::vector<std::pair<AssetID, AssetType>> containedAssets() const override;

    Result<std::unique_ptr<Asset>> buildAsset(const AssetID& id) const override;

    void updateSource(const std::filesystem::path& source);

    void createDefaultSource();
};

#endif // BRANEENGINE_EDITORSCRIPTASSET_H
