#include "asset.h"
#include "assembly.h"
#include "chunk.h"
#include "types/componentAsset.h"
#include "types/imageAsset.h"
#include "types/materialAsset.h"
#include "types/meshAsset.h"
#include "types/shaderAsset.h"
#include <utility/serializedData.h>

void Asset::serialize(OutputSerializer& s) const
{
    AssetID serverlessID = id;
    serverlessID.setAddress("");
    s << serverlessID << name << type.toString();
}

void Asset::deserialize(InputSerializer& s)
{
    std::string typeStr;
    s >> id >> name >> typeStr;
    type.set(typeStr);
}

Asset* Asset::assetFromType(AssetType type)
{
    switch(type.type())
    {
        case AssetType::none:
            throw std::runtime_error("Can't deserialize none type");
            break;
        case AssetType::component:
            return new ComponentAsset();
        case AssetType::system:
            assert("Not implemented" && false);
            break;
        case AssetType::mesh:
            return new MeshAsset();
        case AssetType::image:
            return new ImageAsset();
            break;
        case AssetType::shader:
            return new ShaderAsset();
        case AssetType::material:
            return new MaterialAsset();
        case AssetType::assembly:
            return new Assembly();
        case AssetType::chunk:
            return new WorldChunk();
        case AssetType::player:
            assert("Not implemented" && false);
            break;
    }
    return nullptr;
}

Asset* Asset::deserializeUnknown(InputSerializer& s)
{

    size_t sPos = s.getPos();
    std::string typeStr;
    AssetID id;
    std::string name;
    s >> id >> name >> typeStr;
    AssetType type;
    type.set(typeStr);
    Asset* asset = assetFromType(type);
    s.setPos(sPos);
    if(!asset)
        throw std::runtime_error("unable to create asset type: " + type.toString());
    asset->deserialize(s);
    return asset;
}

std::vector<AssetDependency> Asset::dependencies() const { return {}; }

void Asset::onDependenciesLoaded() {}

void IncrementalAsset::serializeHeader(OutputSerializer& s) const { Asset::serialize(s); }

void IncrementalAsset::deserializeHeader(InputSerializer& s) { Asset::deserialize(s); }

IncrementalAsset* IncrementalAsset::deserializeUnknownHeader(InputSerializer& s)
{
    size_t sPos = s.getPos();
    std::string typeStr;
    AssetID id;
    std::string name;
    s >> id >> name >> typeStr;
    AssetType type;
    type.set(typeStr);
    IncrementalAsset* asset;
    switch(type.type())
    {
        case AssetType::none:
            throw std::runtime_error("Can't deserialize none type");
        case AssetType::mesh:
            asset = new MeshAsset();
            break;
        case AssetType::image:
            // asset = new ImageAsset();
            // break;
        default:
            throw std::runtime_error("Tried to incrementally deserialize, non-incremental asset.");
    }
    s.setPos(sPos);
    asset->deserializeHeader(s);
    return asset;
}

bool IncrementalAsset::serializeIncrement(OutputSerializer& s, SerializationContext* iteratorData) const
{
    return false; // Return false because there is no more data
}

void IncrementalAsset::onFullyLoaded() {}
