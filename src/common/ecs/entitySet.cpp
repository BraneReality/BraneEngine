//
// Created by eli on 7/16/2022.
//

#include "entitySet.h"
#include "archetype.h"
#include "archetypeManager.h"
#include "system.h"

void ComponentFilter::addComponent(ComponentID id, ComponentFilterFlags flags)
{
    _components.push_back({id, flags});
    if(ComponentFilterFlags_Changed & flags || ComponentFilterFlags_Exclude & flags)
        _chunkFlags = true;
}

const std::vector<ComponentFilter::Component>& ComponentFilter::components() const
{
    return _components;
}

ComponentFilter::ComponentFilter(SystemContext* system)
{
    _system = system;
}

SystemContext* ComponentFilter::system() const
{
    return _system;
}

bool ComponentFilter::checkChunk(Chunk* chunk) const
{
    if(!_chunkFlags)
        return true;
    for(auto& comp : _components)
    {
        if(comp.flags & ComponentFilterFlags_Exclude || !(comp.flags & ComponentFilterFlags_Changed))
            continue;
        assert(chunk->hasComponent(comp.id));
        if(chunk->getComponent(comp.id).version <= _system->lastVersion)
            return false;
    }
    return true;
}

bool ComponentFilter::checkArchetype(Archetype* arch) const
{
    for(auto& c : _components)
    {
        bool hasComponent = arch->hasComponent(c.id);
        bool exclude = c.flags & ComponentFilterFlags_Exclude;
        if(hasComponent == exclude)
            return false;
    }
    return true;
}

EntitySet::EntitySet(std::vector<Archetype*> archetypes, ComponentFilter filter)
    : _archetypes(std::move(archetypes)), _filter(std::move(filter))
{}

void EntitySet::forEachNative(const std::function<void(uint8_t** components)>& f)
{
    std::vector<ComponentID> itrComponents;
    for(auto& c : _filter.components())
        if(!(c.flags & ComponentFilterFlags_Exclude))
            itrComponents.push_back(c.id);

    std::vector<ChunkComponentView*> componentViews(itrComponents.size());
    std::vector<uint8_t*> data(itrComponents.size());

    for(auto* arch : _archetypes)
    {
        for(auto& chunk : arch->chunks())
        {
            if(!_filter.checkChunk(chunk.get()))
                continue;

            for(size_t i = 0; i < itrComponents.size(); ++i)
            {
                componentViews[i] = &chunk->getComponent(itrComponents[i]);
                componentViews[i]->lock();
            }
            for(size_t i = 0; i < chunk->size(); ++i)
            {
                for(size_t d = 0; d < itrComponents.size(); ++d)
                    data[d] = componentViews[d]->getComponentData(i);
                f(data.data());
            }
            for(auto c : componentViews)
            {
                c->version = _filter.system()->version;
                c->unlock();
            }
        }
    }
}

size_t EntitySet::archetypeCount() const
{
    return _archetypes.size();
}
