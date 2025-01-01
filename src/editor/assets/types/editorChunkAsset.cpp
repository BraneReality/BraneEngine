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
    auto res = AssetID::parse(_json["id"].asString());
    if(!res)
    {
        Runtime::error(std::format("Script Asset at {} has invalid id", _file.string()));
        return c;
    }
    c.emplace_back(res.ok(), AssetType::chunk);
    return std::move(c);
}

Asset* EditorChunkAsset::buildAsset(const AssetID& id) const
{
    WorldChunk* chunk = new WorldChunk();

    chunk->name = name();
    chunk->id = AssetID::parse(_json["id"].asString()).ok();
    chunk->maxLOD = 0;
    for(auto& lod : _json["LODs"])
    {
        WorldChunk::LOD newLOD;
        newLOD.assembly = AssetID::parse(lod["assembly"].asString()).ok();
        newLOD.min = lod["min"].asUInt();
        newLOD.max = lod["max"].asUInt();
        chunk->maxLOD = std::max(chunk->maxLOD, newLOD.max);
        chunk->LODs.push_back(std::move(newLOD));
    }

    return chunk;
}
