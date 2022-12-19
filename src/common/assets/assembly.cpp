//
// Created by eli on 2/2/2022.
//

#include "assembly.h"
#include "assetManager.h"
#include "common/ecs/componentManager.h"
#include "scripting/transforms.h"
#include <ecs/nativeTypes/meshRenderer.h>
#include "types/meshAsset.h"
#include "types/materialAsset.h"
#include "utility/serializedData.h"


void Assembly::EntityAsset::serialize(OutputSerializer& message, robin_hood::unordered_map<std::string, uint32_t>& componentIndices) const
{
    uint32_t size = static_cast<uint32_t>(components.size());
    message << size;
    for(auto& comp : components)
    {
        auto* desc = comp.description();
        if(!componentIndices.contains(desc->name))
        {
            Runtime::error("Component: " + desc->name + " not found in asset component list");
            throw std::runtime_error("Component assetID not listed in asset components");
        }

        uint32_t ssize = desc->serializationSize();
        message << componentIndices.at(desc->name) << ssize;
        desc->serialize(message, comp.data());
    }
}

void Assembly::EntityAsset::deserialize(InputSerializer& message, Assembly& assembly, ComponentManager& cm, AssetManager& am)
{
    uint32_t size;
    message.readSafeArraySize(size);
    components.reserve(size);
    for (uint32_t i = 0; i < size; ++i)
    {
        uint32_t componentIDIndex, componentSize;
        message >> componentIDIndex >> componentSize;
        if(componentIDIndex >= assembly.components.size())
            throw std::runtime_error("Component ID index invalid");
        const ComponentDescription* description = cm.getComponentDef(assembly.components[componentIDIndex]);
        if(!description)
            throw std::runtime_error("Could not locate component " + assembly.components[componentIDIndex]);
        if(componentSize != description->serializationSize())
        {
            Runtime::error("Component size " + std::to_string(componentSize) + " does not match size of component "
            + description->name + " which is " + std::to_string(description->serializationSize()) + ".\n attempting to skip and continue deserialization");
            message.setPos(message.getPos() + componentSize);
            continue;
        }
        VirtualComponent component(description);
        description->deserialize(message, component.data());
        components.push_back(std::move(component));
    }
}

bool Assembly::EntityAsset::hasComponent(const ComponentDescription* def) const
{
    for (const auto& c : components)
        if (c.description() == def)
            return true;
    return false;
}

VirtualComponent* Assembly::EntityAsset::getComponent(const ComponentDescription* def)
{
    assert(hasComponent(def));
    for (auto& c : components)
        if (c.description() == def)
            return &c;
    return nullptr;
}

std::vector<ComponentID> Assembly::EntityAsset::runtimeComponentIDs()
{
    std::vector<ComponentID> componentIDs;
    componentIDs.reserve(components.size());
    for(auto& component : components)
    {
        componentIDs.push_back(component.description()->id);
    }
    return componentIDs;
}

void Assembly::serialize(OutputSerializer& message) const
{
    Asset::serialize(message);
    message << components << scripts << meshes << materials << rootIndex;
    message << (uint32_t)entities.size();

    robin_hood::unordered_map<std::string, uint32_t> componentIndices;
    componentIndices.reserve(components.size());
    for (uint32_t i = 0; i < components.size(); ++i)
        componentIndices.insert({components[i], i});

    for (uint32_t i = 0; i < entities.size(); ++i)
    {
        entities[i].serialize(message, componentIndices);
    }
}

void Assembly::deserialize(InputSerializer& message)
{
    ComponentManager& cm = Runtime::getModule<EntityManager>()->components();
    AssetManager& am = *Runtime::getModule<AssetManager>();

    Asset::deserialize(message);
    message >> components >> scripts >> meshes >> materials >> rootIndex;
    uint32_t size;
    message.readSafeArraySize(size);
    entities.resize(size);
    for (uint32_t i = 0; i < entities.size(); ++i)
    {
        entities[i].deserialize(message, *this, cm, am);
    }
}

Assembly::Assembly()
{
    type.set(AssetType::Type::assembly);
}

EntityID Assembly::inject(EntityManager& em, std::vector<EntityID>* entityMapRef)
{
    std::vector<EntityID> entityMap(entities.size());
    for (size_t i = 0; i < entities.size(); ++i)
    {
        EntityAsset& entity = entities[i];
        entityMap[i] = em.createEntity(entity.runtimeComponentIDs());
    }
    EntityID rootID = entityMap[rootIndex];

    for (size_t i = 0; i < entities.size(); ++i)
    {
        EntityAsset& entity = entities[i];
        EntityID id = entityMap[i];
        for(auto& component : entity.components)
            em.setComponent(id, component);
    }

#ifdef CLIENT
    auto* am = Runtime::getModule<AssetManager>();
#endif
    for(size_t i = 0; i < entities.size(); ++i)
    {
        EntityAsset& entity = entities[i];
        EntityID entityId = entityMap[i];
        if(entity.hasComponent(Children::def()))
        {
            auto* cc = em.getComponent<Children>(entityMap[i]);
            auto children =  std::move(cc->children);
            for(auto& child : children)
            {
                EntityID childID = entityMap[child.id];
                if(entityId == childID)
                    throw std::runtime_error("Cannot parent an entity to itself");
                Transforms::setParent(childID, entityId, em);

            }
        }
#ifdef CLIENT
        if(entity.hasComponent(MeshRendererComponent::def()))
        {
            auto* renderer = em.getComponent<MeshRendererComponent>(entityMap[i]);
            auto* mesh = am->getAsset<MeshAsset>(meshes[renderer->mesh].sameOrigin(id));
            renderer->mesh = mesh->runtimeID;
            for(auto& mID : renderer->materials)
            {
                if(materials.size() <= mID || materials[mID].null())
                {
                    mID = -1;
                    continue;
                }
                auto* material = am->getAsset<MaterialAsset>(materials[mID].sameOrigin(id));
                mID = material->runtimeID;
            }
        }
#endif
    }
    if(entityMapRef)
        *entityMapRef = std::move(entityMap);
    return rootID;
}

std::vector<AssetDependency> Assembly::dependencies() const
{
    std::vector<AssetDependency> deps;
    for(auto& id : scripts)
        deps.push_back({id, false});
    for(auto& id : materials)
        if(!id.null())
            deps.push_back({id, false});
    for(auto& id : meshes)
        deps.push_back({id, true});
    return deps;
}




