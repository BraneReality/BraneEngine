//
// Created by eli on 7/17/2022.
//

#ifndef BRANEENGINE_ARCHETYPEMANAGER_H
#define BRANEENGINE_ARCHETYPEMANAGER_H

#include "archetype.h"
#include "entitySet.h"
#include "utility/sharedRecursiveMutex.h"

class ComponentManager;

class ArchetypeManager {
#ifdef TEST_BUILD
  public:
#endif

    std::shared_ptr<ChunkPool> _chunkAllocator;
    // Index 1: number of components, Index 2: archetype
    std::vector<std::vector<std::unique_ptr<Archetype>>> _archetypes;
    std::unordered_map<ComponentID, std::unordered_set<Archetype*>> _compToArch;
    ComponentManager& _componentManager;

  public:
    class iterator {
        ArchetypeManager& _ref;
        size_t _size;
        size_t _archetype;

      public:
        iterator(size_t size, size_t archetype, ArchetypeManager& ref);

        void operator++();

        bool operator!=(const iterator& o) const;

        Archetype& operator*();

        using iterator_category = std::forward_iterator_tag;
        using reference = Archetype&;
        using pointer = Archetype*;
    };

    ArchetypeManager(ComponentManager& componentManager);

    Archetype* getArchetype(const ComponentSet& components);

    Archetype* makeArchetype(const ComponentSet& components);

    void destroyArchetype(Archetype* archetype);

    std::vector<Archetype*> getArchetypes(const ComponentFilter& filter);

    void clear();

    iterator begin();

    iterator end();
};

#endif // BRANEENGINE_ARCHETYPEMANAGER_H
