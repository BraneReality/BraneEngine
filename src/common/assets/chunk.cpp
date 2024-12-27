//
// Created by eli on 1/21/2022.
//

#include "chunk.h"
#include "utility/serializedData.h"

WorldChunk::WorldChunk() { type = AssetType::chunk; }

void WorldChunk::serialize(OutputSerializer& s) const
{
    Asset::serialize(s);
    s << maxLOD << static_cast<uint32_t>(LODs.size());
    for(auto& lod : LODs)
        s << lod.assembly << lod.min << lod.max;
}

void WorldChunk::deserialize(InputSerializer& s)
{
    Asset::deserialize(s);
    uint32_t LODCount;
    s >> maxLOD >> LODCount;
    LODs.resize(LODCount);
    for(uint32_t l = 0; l < LODCount; ++l)
    {
        auto& lod = LODs[l];
        s >> lod.assembly >> lod.min >> lod.max;
    }
}
