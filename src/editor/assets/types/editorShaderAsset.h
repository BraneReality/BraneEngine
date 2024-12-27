//
// Created by eli on 8/19/2022.
//

#ifndef BRANEENGINE_EDITORSHADERASSET_H
#define BRANEENGINE_EDITORSHADERASSET_H

#include "../editorAsset.h"
#include "assets/types/shaderAsset.h"

class EditorShaderAsset : public EditorAsset
{
  public:
    EditorShaderAsset(const std::filesystem::path& file, BraneProject& project);

    std::vector<std::pair<AssetID, AssetType>> containedAssets() const override;

    Asset* buildAsset(const AssetID& id) const override;

    void updateSource(const std::filesystem::path& source);

    void createDefaultSource(ShaderType type);

    ShaderType shaderType() const;
};

#endif // BRANEENGINE_EDITORSHADERASSET_H
