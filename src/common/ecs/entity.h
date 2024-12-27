#pragma once

#include "archetypeManager.h"
#include "chunk.h"
#include "componentManager.h"
#include "nativeComponent.h"
#include "systemManager.h"
#include "utility/sharedRecursiveMutex.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>
#include "common/runtime/runtime.h"
#include "common/utility/staticIndexVector.h"
#include <unordered_map>
#include <unordered_set>

class EntityIDComponent : public NativeComponent<EntityIDComponent>
{
    REGISTER_MEMBERS_1("EntityID", id, "id")

  public:
    EntityID id;
};

class EntityName : public NativeComponent<EntityName>
{
    REGISTER_MEMBERS_1("Name", name, "name")

  public:
    std::string name;
};

struct EntityIndex
{
    Archetype* archetype;
    size_t index;
    uint32_t version;
};

class EntityManager : public Module
{
    struct SystemContext
    {
        uint32_t version;
        uint32_t lastVersion;
    };
#ifdef TEST_BUILD
  public:
#else
  private:
#endif
    uint32_t _globalEntityVersion = 0;
    staticIndexVector<EntityIndex> _entities;

    ComponentManager _components;
    ArchetypeManager _archetypes;
    SystemManager _systems;

  public:
    EntityManager();

    EntityManager(const EntityManager&) = delete;

    ~EntityManager();

    Archetype* getArchetype(const ComponentSet& components);

    EntityID createEntity();

    EntityID createEntity(ComponentSet components);

    bool tryGetEntity(size_t index, EntityID& id) const;

    void createEntities(const ComponentSet& components, size_t count);

    void destroyEntity(EntityID entity);

    bool entityExists(EntityID entity) const;

    Archetype* getEntityArchetype(EntityID entity) const;

    bool hasArchetype(EntityID entity) const;

    bool hasComponent(EntityID entity, ComponentID component) const;

    template<class T>
    bool hasComponent(EntityID entity) const
    {
        return hasComponent(entity, T::def()->id);
    }

    void addComponent(EntityID entity, ComponentID component);

    template<class T>
    void addComponent(EntityID entity)
    {
        addComponent(entity, T::def()->id);
    };

    VirtualComponentView getComponent(EntityID entity, ComponentID component) const;

    template<class T>
    T* getComponent(EntityID entity) const
    {
        return T::fromVirtual(getComponent(entity, T::def()->id));
    }

    void setComponent(EntityID entity, const VirtualComponent& component);

    void setComponent(EntityID entity, const VirtualComponentView& component);

    void markComponentChanged(EntityID entity, ComponentID component);

    template<class T>
    void removeComponent(EntityID entity)
    {
        removeComponent(entity, T::def()->id);
    }

    void removeComponent(EntityID entity, ComponentID component);

    ComponentManager& components();

    SystemManager& systems();

    ArchetypeManager& archetypes();

    EntitySet getEntities(ComponentFilter filter);

    static const char* name();

    void stop() override;
};