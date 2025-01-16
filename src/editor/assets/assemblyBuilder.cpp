//
// Created by eli on 2/1/2022.
//

#include "assemblyBuilder.h"
#include "assets/types/materialAsset.h"
#include "common/ecs/nativeTypes/meshRenderer.h"
#include "common/systems/transforms.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

AssemblyBuilder::AssemblyAssets
AssemblyBuilder::buildAssembly(const std::string& name, GLTFLoader& loader, MaterialAsset* defaultMaterial)
{
    AssemblyAssets assets;

    std::unique_ptr<Assembly> assembly = std::make_unique<Assembly>();
    assembly->name = name;

    std::vector<MeshAsset*> meshes = loader.extractAllMeshes();
    for(auto mesh : meshes)
    {
        assets.meshes.push_back(std::unique_ptr<MeshAsset>(mesh));
    }

    std::vector<Assembly::EntityAsset> entities;
    for(Json::Value& node : loader.nodes())
    {
        Assembly::EntityAsset entity;

        if(node.isMember("name"))
        {
            EntityName nameComponent;
            nameComponent.name = node["name"].asString();
            entity.components.emplace_back(nameComponent.toVirtual());
        }

        glm::mat4 transform = glm::mat4(1);
        if(node.isMember("matrix"))
        {

            for(uint8_t i = 0; i < node["matrix"].size(); ++i)
            {
                transform[i / 4][i % 4] = node["matrix"][i].asFloat();
            }
            VirtualComponent tc(Transform::def());
            tc.setVar(0, transform);
            entity.components.push_back(tc);
        }
        else
        {
            TRS trs;
            glm::mat4 translation(1);
            glm::mat4 rotation(1);
            glm::mat4 scale(1);
            if(node.isMember("rotation"))
            {
                glm::quat value = glm::quat();
                value.w = node["rotation"][3].asFloat();
                value.x = node["rotation"][0].asFloat();
                value.y = node["rotation"][1].asFloat();
                value.z = node["rotation"][2].asFloat();
                trs.rotation = value;
                rotation = glm::mat4_cast(value);
            }
            if(node.isMember("scale"))
            {
                glm::vec3 value;
                for(uint8_t i = 0; i < 3; ++i)
                {
                    value[i] = node["scale"][i].asFloat();
                }
                trs.scale = value;
                scale = glm::scale(glm::mat4(1), value);
            }
            if(node.isMember("translation"))
            {
                glm::vec3 value;
                for(uint8_t i = 0; i < node["translation"].size(); ++i)
                {
                    value[i] = node["translation"][i].asFloat();
                }
                trs.translation = value;
                translation = glm::translate(glm::mat4(1), value);
            }
            transform = translation * rotation * scale;
            VirtualComponent tc(Transform::def());
            tc.setVar(0, transform);
            entity.components.push_back(tc);
            entity.components.emplace_back(trs.toVirtual());
        }

        if(node.isMember("mesh"))
        {
            uint32_t meshIndex = node["mesh"].asUInt();

            VirtualComponent mc(MeshRendererComponent::def());
            mc.setVar(0, meshIndex);
            inlineUIntArray& materials = *mc.getVar<inlineUIntArray>(1);
            for(auto& primitive : loader.data()["meshes"][meshIndex]["primitives"])
                materials.push_back(0); // primitive["material"].asUInt()); TODO: extract materials

            entity.components.push_back(std::move(mc));
        }
        entities.push_back(entity);
    }

    // Link up children
    uint32_t pIndex = 0;
    for(Json::Value& node : loader.nodes())
    {
        if(node.isMember("children"))
        {
            Children cc;
            for(auto& child : node["children"])
            {
                if(child.asUInt() == pIndex)
                    throw std::runtime_error("Cannot parent entity to itself");
                Assembly::EntityAsset& childEnt = entities[child.asUInt()];
                glm::mat4 localTransform = childEnt.getComponent(Transform::def())->readVar<glm::mat4>(0);

                LocalTransform tc{};
                tc.value = localTransform;
                tc.parent = pIndex;
                childEnt.components.emplace_back(tc.toVirtual());
                cc.children.push_back({child.asUInt(), 0});
            }
            entities[pIndex].components.emplace_back(cc.toVirtual());
        }
        pIndex++;
    }

    // Register component ids
    assembly->components.push_back(EntityName::def()->asset->id);
    assembly->components.push_back(Transform::def()->asset->id);
    assembly->components.push_back(LocalTransform::def()->asset->id);
    assembly->components.push_back(Children::def()->asset->id);
    assembly->components.push_back(TRS::def()->asset->id);
    assembly->components.push_back(MeshRendererComponent::def()->asset->id);

    assembly->materials.push_back(defaultMaterial->id);

    assembly->entities = std::move(entities);
    assets.assembly = std::move(assembly);
    return std::move(assets);
}
