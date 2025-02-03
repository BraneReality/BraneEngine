//
// Created by eli on 9/27/2022.
//

#include "assemblyReloadManager.h"
#include "assets/assembly.h"
#include "assets/assetManager.h"
#include "assets/types/materialAsset.h"
#include "assets/types/meshAsset.h"
#include "ecs/entity.h"
#include "ecs/nativeTypes/meshRenderer.h"
#include "systems/transforms.h"

AssemblyReloadManager::AssemblyReloadManager()
{
    _em = Runtime::getModule<EntityManager>();
}

const char* AssemblyReloadManager::name()
{
    return "assemblyReloadManager";
}

EntityID AssemblyReloadManager::instantiate(Assembly* assembly)
{
    std::scoped_lock lock(m);
    AssemblyInstance instance;
    EntityID rootID = assembly->inject(*_em, &instance.entities);
    instance.root = rootID;
    _instances[assembly].push_back(std::move(instance));
    return rootID;
}

void AssemblyReloadManager::destroy(Assembly* assembly, EntityID assemblyRoot)
{
    std::scoped_lock lock(m);
    auto& instanceSet = _instances[assembly];
    for(auto instance = instanceSet.begin(); instance != instanceSet.end(); instance++)
    {
        if(instance->root == assemblyRoot)
        {
            Runtime::getModule<Transforms>()->destroyRecursive(assemblyRoot);
            instanceSet.erase(instance);
            return;
        }
    }
}

void AssemblyReloadManager::updateEntityComponent(Assembly* assembly, size_t index, VirtualComponentView component)
{
    std::scoped_lock lock(m);
    auto* am = Runtime::getModule<AssetManager>();
    VirtualComponent newData = component;
    for(auto& i : _instances[assembly])
    {
        if(newData.description() == MeshRendererComponent::def())
        {
            auto* renderer = MeshRendererComponent::fromVirtual(newData);
            renderer->mesh = am->getAsset<MeshAsset>(assembly->meshes[renderer->mesh]).value()->runtimeID;
            for(auto& material : renderer->materials)
            {
                AssetID id;
                if(material < assembly->materials.size())
                    id = assembly->materials[material];
                if(!id.empty())
                    material = am->getAsset<MaterialAsset>(id).value()->runtimeID;
                else
                    material = -1;
            }
        }
        else if(newData.description() == LocalTransform::def())
        {
            auto* lt = LocalTransform::fromVirtual(newData);
            lt->parent = i.entities[lt->parent.id];
        }
        else if(newData.description() == Children::def())
        {
            auto* children = Children::fromVirtual(newData);
            for(auto& child : children->children)
                child = i.entities[child.id];
        }
        _em->setComponent(i.entities[index], newData);
        _em->markComponentChanged(i.entities[index], newData.description()->id);
    }
}

void AssemblyReloadManager::updateEntityParent(Assembly* assembly, size_t entity, size_t parent)
{
    std::scoped_lock lock(m);
    assert(assembly->rootIndex != entity);
    for(auto& i : _instances[assembly])
        Transforms::setParent(i.entities[entity], i.entities[parent], *_em, false);
}

void AssemblyReloadManager::addEntityComponent(Assembly* assembly, size_t index, VirtualComponentView component)
{
    {
        std::scoped_lock lock(m);
        for(auto& i : _instances[assembly])
            _em->addComponent(i.entities[index], component.description()->id);
    }
    updateEntityComponent(assembly, index, component);
}

void AssemblyReloadManager::removeEntityComponent(Assembly* assembly, size_t index, ComponentID compID)
{
    std::scoped_lock lock(m);
    for(auto& i : _instances[assembly])
        _em->removeComponent(i.entities[index], compID);
}

void AssemblyReloadManager::insertEntity(Assembly* assembly, size_t index)
{
    std::scoped_lock lock(m);
    for(auto& i : _instances[assembly])
    {
        EntityID newEnt = _em->createEntity();
        i.entities.insert(i.entities.begin() + index, newEnt);
    }
}

void AssemblyReloadManager::reorderEntity(Assembly* assembly, size_t before, size_t after)
{
    std::scoped_lock lock(m);
    if(before == after)
        return;
    for(auto& i : _instances[assembly])
    {
        EntityID entity = i.entities[before];
        i.entities.erase(i.entities.begin() + before);
        i.entities.insert(i.entities.begin() + after, entity);
    }
}

void AssemblyReloadManager::removeEntity(Assembly* assembly, size_t index)
{
    std::scoped_lock lock(m);
    for(auto& i : _instances[assembly])
    {
        Transforms::removeParent(i.entities[index], *_em, false);
        _em->destroyEntity(i.entities[index]);
        i.entities.erase(i.entities.begin() + index);
    }
}

EntityID AssemblyReloadManager::getEntity(Assembly* assembly, size_t instance, size_t entity)
{
    return _instances[assembly][instance].entities[entity];
}
