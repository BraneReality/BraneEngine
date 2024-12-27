//
// Created by eli on 9/27/2022.
//

#ifndef BRANEENGINE_ASSEMBLYRELOADMANAGER_H
#define BRANEENGINE_ASSEMBLYRELOADMANAGER_H

#include "ecs/component.h"
#include "ecs/entityID.h"
#include "runtime/runtime.h"

class Assembly;

class EntityManager;

class AssemblyReloadManager : public Module
{
    struct AssemblyInstance
    {
        EntityID root;
        std::vector<EntityID> entities;
    };

    std::unordered_map<Assembly*, std::vector<AssemblyInstance>> _instances;
    std::mutex m;
    EntityManager* _em;

  public:
    AssemblyReloadManager();

    EntityID instantiate(Assembly* assembly);

    void destroy(Assembly* assembly, EntityID assemblyRoot);

    EntityID getEntity(Assembly* assembly, size_t instance, size_t entity);

    void updateEntityComponent(Assembly* assembly, size_t index, VirtualComponentView component);

    void updateEntityParent(Assembly* assembly, size_t entity, size_t parent);

    void addEntityComponent(Assembly* assembly, size_t index, VirtualComponentView component);

    void removeEntityComponent(Assembly* assembly, size_t index, ComponentID compID);

    void insertEntity(Assembly* assembly, size_t index);

    void reorderEntity(Assembly* assembly, size_t before, size_t after);

    void removeEntity(Assembly* assembly, size_t index);

    static const char* name();
};

#endif // BRANEENGINE_ASSEMBLYRELOADMANAGER_H
