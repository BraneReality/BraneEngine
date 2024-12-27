//
// Created by eli on 11/30/2022.
//

#ifndef BRANEENGINE_EDITORIMAGEASSET_H
#define BRANEENGINE_EDITORIMAGEASSET_H

#include "../editorAsset.h"

class EditorImageAsset : public EditorAsset
{
  public:
    EditorImageAsset(const std::filesystem::path& file, BraneProject& project);

    std::vector<std::pair<AssetID, AssetType>> containedAssets() const override;

    Asset* buildAsset(const AssetID& id) const override;

    void updateSource(const std::filesystem::path& source);
};

#endif // BRANEENGINE_EDITORIMAGEASSET_H
