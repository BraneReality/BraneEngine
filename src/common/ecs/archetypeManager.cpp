//
// Created by eli on 7/17/2022.
//

#include "archetypeManager.h"
#include "componentManager.h"
#include <unordered_set>

ArchetypeManager::ArchetypeManager(ComponentManager& componentManager) : _componentManager(componentManager)
{
    _chunkAllocator = std::make_shared<ChunkPool>();
}

Archetype* ArchetypeManager::getArchetype(const ComponentSet& components)
{
    ASSERT_MAIN_THREAD();
    size_t numComps = components.size();
    assert(numComps > 0);
    if(numComps > _archetypes.size())
        return makeArchetype(components);
    std::vector<std::unordered_set<Archetype*>*> archesWithComponent;
    for(ComponentID c : components) {
        if(_compToArch.count(c))
            archesWithComponent.push_back(&_compToArch[c]);
        else
            return makeArchetype(components);
    }

    // Find smallest, (basically we never want to use entityID as the iterator in the next for-each
    size_t smallest = 0;
    for(size_t i = 1; i < archesWithComponent.size(); ++i)
        if(archesWithComponent[smallest]->size() > archesWithComponent[i]->size())
            smallest = i;

    Archetype* foundArch = nullptr;
    for(auto* arch : *archesWithComponent[smallest]) {
        if(arch->components().size() != components.size())
            continue;
        bool isFound = true;
        for(size_t i = 0; i < archesWithComponent.size(); ++i) {
            if(i == smallest)
                continue;
            if(!archesWithComponent[i]->contains(arch)) {
                isFound = false;
                break;
            }
        }
        if(isFound) {
            foundArch = arch;
            break;
        }
    }

    if(foundArch)
        return foundArch;

    return makeArchetype(components);
}

Archetype* ArchetypeManager::makeArchetype(const ComponentSet& components)
{
    ASSERT_MAIN_THREAD();
    assert(components.size() > 0);
    size_t numComps = components.size();

    // We want to keep archetypes of the same size in groups
    // This means we keep each size in a different vector

    // Make sure we have enough vectors
    if(numComps > _archetypes.size())
        _archetypes.resize(numComps);

    std::vector<const ComponentDescription*> descriptions;
    descriptions.reserve(components.size());
    for(auto id : components)
        descriptions.push_back(_componentManager.getComponentDef(id));

    size_t newIndex = _archetypes[numComps - 1].size();
    _archetypes[numComps - 1].push_back(std::make_unique<Archetype>(descriptions, _chunkAllocator));

    Archetype* newArch = _archetypes[numComps - 1][newIndex].get();

    // find edges to other archetypes lower than this one
    if(numComps > 1) {
        ComponentID connectingComponent;
        for(size_t i = 0; i < _archetypes[numComps - 2].size(); i++) {
            if(_archetypes[numComps - 2][i]->isChildOf(newArch, connectingComponent)) {
                Archetype* otherArch = _archetypes[numComps - 2][i].get();
                otherArch->addEdges().insert({connectingComponent, newArch});
                newArch->removeEdges().insert({connectingComponent, otherArch});
            }
        }
    }

    // find edges to other archetypes higher than this one
    if(_archetypes.size() > numComps) {
        for(size_t i = 0; i < _archetypes[numComps].size(); i++) {
            ComponentID connectingComponent;
            if(newArch->isChildOf(_archetypes[numComps][i].get(), connectingComponent)) {
                Archetype* otherArch = _archetypes[numComps][i].get();
                newArch->addEdges().insert({connectingComponent, otherArch});
                otherArch->removeEdges().insert({connectingComponent, newArch});
            }
        }
    }

    for(auto c : components)
        _compToArch[c].insert(newArch);

    return newArch;
}

void ArchetypeManager::destroyArchetype(Archetype* archetype)
{
    assert(archetype);
    ASSERT_MAIN_THREAD();
    for(auto c : archetype->components())
        _compToArch[c].erase(archetype);

    for(auto& edge : archetype->addEdges())
        edge.second->removeEdges().erase(edge.first);
    for(auto& edge : archetype->removeEdges())
        edge.second->addEdges().erase(edge.first);
    auto& archesOfSameSize = _archetypes[archetype->components().size() - 1];
    auto i = archesOfSameSize.begin();
    auto end = archesOfSameSize.end();
    while(i != end) {
        if(i->get() == archetype) {
            archesOfSameSize.erase(i);
            return;
        }
        ++i;
    }
    throw std::runtime_error("Could not find archetype to delete");
}

void ArchetypeManager::clear()
{
    ASSERT_MAIN_THREAD();
    _archetypes.resize(0);
}

std::vector<Archetype*> ArchetypeManager::getArchetypes(const ComponentFilter& filter)
{
    assert(filter.components().size() > 0);
    std::vector<Archetype*> archetypes;
    archetypes.reserve(64);

    std::vector<std::unordered_set<Archetype*>*> archesWithDesiredComponents;
    archesWithDesiredComponents.reserve(filter.components().size());
    std::vector<std::unordered_set<Archetype*>*> archesWithExcludedComponents;
    for(auto c : filter.components()) {
        if(c.flags & ComponentFilterFlags_Exclude)
            archesWithExcludedComponents.push_back(&_compToArch[c.id]);
        else
            archesWithDesiredComponents.push_back(&_compToArch[c.id]);
    }

    // Find smallest, (basically we never want to use entityID as the iterator in the next for loop)
    uint32_t s = 0;
    for(uint32_t i = 1; i < archesWithDesiredComponents.size(); ++i)
        if(archesWithDesiredComponents[s]->size() > archesWithDesiredComponents[i]->size())
            s = i;
    auto& smallestSet = *archesWithDesiredComponents[s];
    archesWithDesiredComponents.erase(archesWithDesiredComponents.begin() + s);

    for(auto arch : smallestSet) {
        for(auto& cSet : archesWithDesiredComponents)
            if(!cSet->contains(arch))
                goto nextArch;
        for(auto& cSet : archesWithExcludedComponents)
            if(cSet->contains(arch))
                goto nextArch;
        archetypes.push_back(arch);
    nextArch:
        continue;
    }

    return archetypes;
}

ArchetypeManager::iterator ArchetypeManager::begin() { return {0, 0, *this}; }

ArchetypeManager::iterator ArchetypeManager::end() { return {_archetypes.size(), 0, *this}; }

ArchetypeManager::iterator::iterator(size_t size, size_t archetype, ArchetypeManager& ref)
    : _size{size}, _archetype{archetype}, _ref(ref)
{
    while(_size < _ref._archetypes.size() && _archetype == _ref._archetypes[_size].size()) {
        _archetype = 0;
        _size++;
    }
}

void ArchetypeManager::iterator::operator++()
{
    _archetype++;
    while(_size < _ref._archetypes.size() && _archetype == _ref._archetypes[_size].size()) {
        _archetype = 0;
        _size++;
    }
}

bool ArchetypeManager::iterator::operator!=(const ArchetypeManager::iterator& o) const
{
    return !(_size == o._size && _archetype == o._archetype);
}

Archetype& ArchetypeManager::iterator::operator*() { return *_ref._archetypes[_size][_archetype]; }
