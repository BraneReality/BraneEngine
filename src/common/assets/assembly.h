//
// Created by eli on 2/2/2022.
//

#ifndef BRANEENGINE_ASSEMBLY_H
#define BRANEENGINE_ASSEMBLY_H

#include "asset.h"
#include <json/json.h>
#include "ecs/component.h"
#include "robin_hood.h"

class EntityManager;
class ComponentManager;
class AssetManager;
namespace graphics{
    class VulkanRuntime;
}

class Assembly : public Asset
{
public:
    struct EntityAsset
    {
        std::vector<VirtualComponent> components;
        std::vector<ComponentID> runtimeComponentIDs();
        void serialize(OutputSerializer& message, robin_hood::unordered_map<std::string, uint32_t>& componentIndices) const;
        void deserialize(InputSerializer& message, Assembly& assembly, ComponentManager& cm, AssetManager& am);
        bool hasComponent(const ComponentDescription* def) const;
        VirtualComponent* getComponent(const ComponentDescription* def);
    };

    Assembly();
    std::vector<std::string> components;
    std::vector<AssetID> scripts; // Any systems in dependencies will be automatically loaded
    std::vector<AssetID> meshes; // We need to store these in a list, so we can tell witch asset entities are referring to
    std::vector<AssetID> materials;
    std::vector<EntityAsset> entities;
    uint32_t rootIndex = 0;
    void serialize(OutputSerializer& message) const override;
    void deserialize(InputSerializer& message) override;

    std::vector<AssetDependency> dependencies() const override;
    EntityID inject(EntityManager& em, std::vector<EntityID>* entityMap = nullptr);
};


#endif //BRANEENGINE_ASSEMBLY_H
