#pragma once

#include "assets/asset.h"
#include "ecs/virtualType.h"

class ComponentAsset;

class MaterialAsset : public Asset
{
  public:
    MaterialAsset();

    std::vector<std::pair<uint16_t, AssetID>> textures;
    std::vector<uint8_t> serializedProperties;
    AssetID vertexShader;
    AssetID fragmentShader;

    void serialize(OutputSerializer& s) const override;

    void deserialize(InputSerializer& s) override;

    std::vector<AssetDependency> dependencies() const override;

#ifdef CLIENT
    uint32_t runtimeID = -1;
    void onDependenciesLoaded() override;
#endif
};