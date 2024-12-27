//
// Created by wirewhiz on 27/07/22.
//

#ifndef BRANEENGINE_ENTITYID_H
#define BRANEENGINE_ENTITYID_H

#include <cstdint>

struct EntityID
{
    uint32_t id = 0;
    uint32_t version = -1;

    EntityID& operator=(uint32_t);

    bool operator==(const EntityID& o) const;

    bool operator!=(const EntityID& o) const;
};

#endif // BRANEENGINE_ENTITYID_H
