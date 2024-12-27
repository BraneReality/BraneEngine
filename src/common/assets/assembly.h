//
// Created by eli on 2/2/2022.
//

#ifndef BRANEENGINE_ASSEMBLY_H
#define BRANEENGINE_ASSEMBLY_H

#include "asset.h"
#include "ecs/component.h"
#include <json/json.h>

class EntityManager;

class ComponentManager;

class AssetManager;

namespace graphics
{
    class VulkanRuntime;
}

class Assembly : public Asset
{
  public:
    struct EntityAsset
    {
        std::vector<VirtualComponent> components;

        std::vector<ComponentID> runtimeComponentIDs();

        void serialize(OutputSerializer& message, const Assembly& assembly) const;

        void deserialize(InputSerializer& message, Assembly& assembly, ComponentManager& cm, AssetManager& am);

        bool hasComponent(const ComponentDescription* def) const;

        VirtualComponent* getComponent(const ComponentDescription* def);
    };

    Assembly();

    std::vector<AssetID> components;
    std::vector<AssetID> scripts; // Any systems in dependencies will be automatically loaded
    std::vector<AssetID>
        meshes; // We need to store these in a list, so we can tell witch asset entities are referring to
    std::vector<AssetID> materials;
    std::vector<EntityAsset> entities;
    uint32_t rootIndex = 0;

    void serialize(OutputSerializer& message) const override;

    void deserialize(InputSerializer& message) override;

    std::vector<AssetDependency> dependencies() const override;

    void onDependenciesLoaded() override;

    EntityID inject(EntityManager& em, std::vector<EntityID>* entityMap = nullptr);
};

#endif // BRANEENGINE_ASSEMBLY_H
