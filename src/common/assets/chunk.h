//
// Created by eli on 1/21/2022.
//

#ifndef BRANEENGINE_CHUNK_H
#define BRANEENGINE_CHUNK_H

#include "assembly.h"
#include "asset.h"
#include "common/ecs/component.h"

class WorldChunk : public Asset
{
  public:
    struct LOD
    {
        AssetID assembly;
        uint32_t max = -1;
        uint32_t min = -1;
    };

    std::vector<LOD> LODs;
    uint32_t maxLOD = -1;

    WorldChunk();

    virtual void serialize(OutputSerializer& s) const;

    virtual void deserialize(InputSerializer& s);
};

#endif // BRANEENGINE_CHUNK_H
