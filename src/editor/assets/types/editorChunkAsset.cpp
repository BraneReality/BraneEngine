//
// Created by eli on 9/26/2022.
//

#include "editorChunkAsset.h"
#include "assets/chunk.h"

EditorChunkAsset::EditorChunkAsset(const std::filesystem::path& file, BraneProject& project)
    : EditorAsset(file, project)
{
    // Generate default
    if(!std::filesystem::exists(_file))
    {
        Json::Value defaultLOD;
        defaultLOD["assembly"] = "null";
        defaultLOD["min"] = 0;
        defaultLOD["max"] = 0;
        _json.data()["LODs"].append(defaultLOD);
    }
}

std::vector<std::pair<AssetID, AssetType>> EditorChunkAsset::containedAssets() const
{
    std::vector<std::pair<AssetID, AssetType>> c;
    c.emplace_back(AssetID(_json["id"].asString()), AssetType::chunk);
    return std::move(c);
}

Asset* EditorChunkAsset::buildAsset(const AssetID& id) const
{
    WorldChunk* chunk = new WorldChunk();

    chunk->name = name();
    chunk->id = _json["id"].asString();
    chunk->maxLOD = 0;
    for(auto& lod : _json["LODs"])
    {
        WorldChunk::LOD newLOD;
        newLOD.assembly = lod["assembly"].asString();
        newLOD.min = lod["min"].asUInt();
        newLOD.max = lod["max"].asUInt();
        chunk->maxLOD = std::max(chunk->maxLOD, newLOD.max);
        chunk->LODs.push_back(std::move(newLOD));
    }

    return chunk;
}
