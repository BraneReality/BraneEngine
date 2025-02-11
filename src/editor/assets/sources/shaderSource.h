#pragma once
#include <filesystem>
#include "assetSource.h"
#include "editor/graphics/shaderCompiler.h"

struct ShaderAssetSource : public AssetSource
{
    ShaderCompiler::ShaderAttributes attributes;

    ShaderAssetSource(std::filesystem::path path);
    std::vector<std::pair<AssetSourceID, AssetType>> exportedAssets() const override;
    Result<std::shared_ptr<AssetMetadata>> buildMetadata(const AssetSourceID& id) const override;
    Result<std::shared_ptr<Asset>> buildAsset(std::shared_ptr<AssetMetadata> metadata) const override;
    Result<void> save() override;
    AssetSourceType type() override;

    void updateAttributes();
    ShaderType shaderType() const;
};

struct ShaderAssetMetadata : public AssetMetadata
{
    ShaderAssetMetadata();
    std::string typeName() const override;
    Result<Json::Value> serialize() const override;
    AssetMetadataType type() override;
    AssetType assetType() const override;
};
