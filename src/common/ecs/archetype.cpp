#include "archetype.h"

#include "chunk.h"
#include "entitySet.h"

size_t Archetype::chunkIndex(size_t entity) const { return entity / (Chunk::allocationSize() / _entitySize); }

Archetype::Archetype(
    const std::vector<const ComponentDescription*>& components, std::shared_ptr<ChunkPool>& chunkAllocator)
{
    _chunkAllocator = chunkAllocator;
    _entitySize = 0;
    _componentDescriptions = components;
    for(auto component : components) {
        _components.add(component->id);
        _entitySize += component->size();
    }
}

Archetype::~Archetype()
{
    while(!_chunks.empty()) {
        *_chunkAllocator << _chunks[_chunks.size() - 1];
        _chunks.erase(_chunks.end() - 1);
    }
}

bool Archetype::hasComponent(ComponentID component) const { return _components.contains(component); }

bool Archetype::hasComponents(const ComponentSet& comps) const { return _components.contains(comps); }

bool Archetype::hasComponents(const ComponentFilter& filter) const
{
    for(auto& c : filter.components())
        if(hasComponent(c.id))
            return true;
    return true;
}

VirtualComponentView Archetype::getComponent(size_t entity, ComponentID component) const
{
    assert(_components.contains(component));

    size_t chunk = chunkIndex(entity);
    assert(chunk < _chunks.size());

    size_t index = entity - chunk * _chunks[0]->maxCapacity();
    assert(index < _chunks[chunk]->size());
    VirtualComponentView o = _chunks[chunk]->getComponent(component)[index];
    return o;
}

void Archetype::setComponent(size_t entity, VirtualComponent&& component)
{
    assert(_components.contains(component.description()->id));

    size_t chunk = chunkIndex(entity);
    assert(chunk < _chunks.size());

    size_t index = entity - chunk * _chunks[0]->maxCapacity();
    assert(index < _chunks[chunk]->size());
    auto& componentView = _chunks[chunk]->getComponent(component.description()->id);
    componentView.setComponent(index, std::move(component));
    componentView.version++;
}

void Archetype::setComponent(size_t entity, VirtualComponentView component)
{
    assert(_components.contains(component.description()->id));

    size_t chunk = chunkIndex(entity);
    assert(chunk < _chunks.size());

    size_t index = entity - chunk * _chunks[0]->maxCapacity();
    assert(index < _chunks[chunk]->size());
    auto& componentView = _chunks[chunk]->getComponent(component.description()->id);
    componentView.setComponent(index, component);
    componentView.version++;
}

bool Archetype::isChildOf(const Archetype* parent, ComponentID& connectingComponent) const
{
    connectingComponent = -1;
    assert(_components.size() + 1 == parent->_components.size()); // Make sure this is a valid comparison
    byte missCount = 0;
    for(auto& c : parent->_components) {
        if(!_components.contains(c)) {
            if(++missCount == 2)
                return false;
            connectingComponent = c;
        }
    }
    return true;
}

const ComponentSet& Archetype::components() const { return _components; }

const std::vector<const ComponentDescription*>& Archetype::componentDescriptions() { return _componentDescriptions; }

std::unordered_map<ComponentID, Archetype*>& Archetype::addEdges() { return _addEdges; }

std::unordered_map<ComponentID, Archetype*>& Archetype::removeEdges() { return _removeEdges; }

size_t Archetype::size() const { return _size; }

size_t Archetype::createEntity()
{
    size_t chunk = chunkIndex(_size);
    if(chunk >= _chunks.size()) {
        _chunks.resize(_chunks.size() + 1);
        *_chunkAllocator >> _chunks[_chunks.size() - 1];
        _chunks[_chunks.size() - 1]->setComponents(_componentDescriptions);
    }

    assert(chunk < _chunks.size());
    _chunks[chunk]->createEntity();

    return _size++;
}

size_t Archetype::moveEntity(size_t index, Archetype* destination)
{
    assert(index < _size);
    size_t srcChunkIndex = chunkIndex(index);
    size_t newIndex = destination->createEntity();
    size_t destChunkIndex = destination->chunkIndex(newIndex);

    Chunk* src = _chunks[srcChunkIndex].get();
    Chunk* dest = destination->_chunks[destChunkIndex].get();

    // Index within chunk
    size_t srcEntityIndex = index - srcChunkIndex * src->maxCapacity();
    size_t destEntityIndex = newIndex - destChunkIndex * dest->maxCapacity();

    src->moveEntity(dest, srcEntityIndex, destEntityIndex);
    removeEntity(index);
    return newIndex;
}

Chunk* Archetype::getChunk(size_t entity) const { return _chunks[chunkIndex(entity)].get(); }

size_t Archetype::entitySize() const { return _entitySize; }

void Archetype::removeEntity(size_t index)
{
    assert(index < _size);
    Chunk* chunk = getChunk(index);
    Chunk* lastChunk = _chunks[_chunks.size() - 1].get();
    size_t entityIndex = index - chunkIndex(index) * chunk->maxCapacity();

    if(chunk == lastChunk)
        chunk->removeEntity(entityIndex);
    else {
        lastChunk->moveEntity(chunk, lastChunk->size() - 1, entityIndex);
        lastChunk->removeEntity(lastChunk->size() - 1);
    }
    --_size;
    assert(lastChunk->size() % lastChunk->maxCapacity() == _size % lastChunk->maxCapacity());

    if(lastChunk->size() == 0) {
        *_chunkAllocator << _chunks[_chunks.size() - 1];
        _chunks.resize(_chunks.size() - 1);
    }
}

const std::vector<std::unique_ptr<Chunk>>& Archetype::chunks() const { return _chunks; }

void Archetype::setComponentVersion(size_t entity, ComponentID component, uint32_t version)
{
    getChunk(entity)->getComponent(component).version = version;
}
