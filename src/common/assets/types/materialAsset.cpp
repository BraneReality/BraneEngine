#include "materialAsset.h"
#include "componentAsset.h"
#include "graphics/graphics.h"
#include <utility/serializedData.h>

void MaterialAsset::serialize(OutputSerializer& s) const
{
    Asset::serialize(s);
    s << serializedProperties << vertexShader << fragmentShader;
    s << (uint16_t)textures.size();
    for(auto& tb : textures)
        s << tb.first << tb.second;
}

void MaterialAsset::deserialize(InputSerializer& s)
{
    Asset::deserialize(s);
    s >> serializedProperties >> vertexShader >> fragmentShader;
    uint16_t textureCount;
    s >> textureCount;
    for(uint16_t i = 0; i < textureCount; ++i)
    {
        std::pair<uint16_t, AssetID> textureBinding;
        s >> textureBinding.first >> textureBinding.second;
        textures.push_back(textureBinding);
    }
}

MaterialAsset::MaterialAsset()
{
    type.set(AssetType::material);
}

std::vector<AssetDependency> MaterialAsset::dependencies() const
{
    std::vector<AssetDependency> deps;
    if(!vertexShader.empty())
        deps.push_back({vertexShader, false});
    if(!fragmentShader.empty())
        deps.push_back({fragmentShader, false});
    for(auto& t : textures)
        deps.push_back({t.second, true});
    return deps;
}

#ifdef CLIENT
void MaterialAsset::onDependenciesLoaded()
{
    auto* vkr = Runtime::getModule<graphics::VulkanRuntime>();
    runtimeID = vkr->addAsset(this);
}

#endif
