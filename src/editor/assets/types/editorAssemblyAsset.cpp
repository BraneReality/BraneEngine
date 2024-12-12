//
// Created by eli on 8/20/2022.
//

#include "editorAssemblyAsset.h"
#include "../gltfLoader.h"
#include "../jsonVirtualType.h"
#include "assets/assembly.h"
#include "assets/assetManager.h"
#include "ecs/nativeTypes/meshRenderer.h"
#include "editor/assets/assemblyReloadManager.h"
#include "editor/braneProject.h"
#include "editor/editorEvents.h"
#include "systems/transforms.h"
#include "ui/gui.h"
#include "utility/jsonTypeUtilities.h"
#include <regex>

Json::Value EditorAssemblyAsset::newEntity(uint32_t parent)
{
    Json::Value newEntity;
    newEntity["name"] = "new entity";
    newEntity["parent"] = parent;
    Transform t;
    TRS trs;
    newEntity["components"].append(EditorAssemblyAsset::componentToJson(t.toVirtual()));
    newEntity["components"].append(EditorAssemblyAsset::componentToJson(trs.toVirtual()));
    return newEntity;
}

EditorAssemblyAsset::EditorAssemblyAsset(const std::filesystem::path& file, BraneProject& project)
    : EditorAsset(file, project)
{
    // Generate default
    if(!std::filesystem::exists(_file)) {
        _json.data()["linked"] = false;
        _json.data()["dependencies"]["meshes"] = Json::arrayValue;
        _json.data()["dependencies"]["materials"] = Json::arrayValue;
        Json::Value rootEntity;
        rootEntity["name"] = "root";
        Transform t;
        rootEntity["components"].append(EditorAssemblyAsset::componentToJson(t.toVirtual()));
        _json.data()["entities"].append(rootEntity);
    }
}

void EditorAssemblyAsset::linkToGLTF(const std::filesystem::path& file)
{
    // To avoid issues of overwriting user changes, only we extract entities once. Users will have to force this by
    // deleting the file manually
    if(_json["linked"].asBool())
        return;

    // Clear default entity
    _json.data()["entities"].clear();

    _json.data()["linked"] = true;
    _json.data()["source"] = (std::filesystem::relative(file, _file.parent_path())).string();

    GLTFLoader gltf;
    gltf.loadFromFile(file);

    for(auto& mesh : gltf.json()["meshes"]) {
        Json::Value meshData;
        meshData["id"] = _project.newAssetID(_file, AssetType::mesh).string();
        meshData["name"] = mesh["name"];
        _json.data()["linkedMeshes"].append(meshData);
        _json.data()["dependencies"]["meshes"].append(meshData["id"]);
    }

    for(auto& node : gltf.json()["nodes"]) {
        Json::Value entity;
        if(node.isMember("name"))
            entity["name"] = node["name"];
        TRS trs;
        if(node.isMember("matrix")) {
            Transform t;
            t.value = fromJson<glm::mat4>(node["matrix"]);
            entity["components"].append(componentToJson(t));
            trs.fromMat(t.value);
        }
        if(node.isMember("translation"))
            trs.translation = fromJson<glm::vec3>(node["translation"]);
        if(node.isMember("rotation"))
            trs.rotation = fromJson<glm::quat>(node["rotation"]);
        if(node.isMember("scale"))
            trs.scale = fromJson<glm::vec3>(node["scale"]);
        entity["components"].append(componentToJson(trs));

        if(node.isMember("mesh")) {
            MeshRendererComponent renderer;
            renderer.mesh = node["mesh"].asUInt();
            for(auto& primitive : gltf.json()["meshes"][renderer.mesh]["primitives"])
                renderer.materials.push_back(primitive["material"].asUInt());
            entity["components"].append(componentToJson(renderer));
        }
        if(node.isMember("children"))
            entity["children"] = node["children"];
        _json.data()["entities"].append(entity);
    }

    _json.data()["rootEntity"] = gltf.json()["scenes"][0]["nodes"].get(Json::ArrayIndex(0), "0");

    Json::ArrayIndex index = 0;
    for(auto& entity : _json.data()["entities"]) {
        // Set parent value on entities
        if(entity.isMember("children")) {
            for(auto& child : entity["children"]) {
                if(index == child.asUInt())
                    throw std::runtime_error("Cannot parent entity to itself");
                _json.data()["entities"][child.asUInt()]["parent"] = index;
            }
        }
        index++;
    }

    auto& materials = _json.data()["dependencies"]["materials"];
    while(materials.size() < gltf.json()["materials"].size())
        materials.append("null");

    save();
}

Asset* EditorAssemblyAsset::buildAsset(const AssetID& id) const
{
    if(id.string() == _json["id"].asString())
        return buildAssembly();
    return buildMesh(id);
}

Json::Value EditorAssemblyAsset::componentToJson(VirtualComponentView component)
{
    Json::Value output;
    output["name"] = component.description()->name;
    output["id"] = component.description()->asset->id.string();

    auto& members = component.description()->asset->members();
    auto& names = component.description()->asset->memberNames();
    for(size_t i = 0; i < members.size(); ++i) {
        Json::Value member;
        member["name"] = names[i];
        member["value"] = JsonVirtualType::fromVirtual(component.getVar<byte>(i), members[i]);
        member["type"] = VirtualType::typeToString(members[i]);
        output["members"].append(member);
    }
    return output;
}

VirtualComponent EditorAssemblyAsset::jsonToComponent(Json::Value component)
{
    auto* am = Runtime::getModule<AssetManager>();
    auto* em = Runtime::getModule<EntityManager>();

    auto compID = am->getAsset<ComponentAsset>(AssetID(component["id"].asString()))->componentID;
    auto* description = em->components().getComponentDef(compID);

    VirtualComponent output(description);

    auto& members = description->asset->members();
    auto& names = description->asset->memberNames();
    for(Json::ArrayIndex i = 0; i < members.size(); ++i)
        JsonVirtualType::toVirtual(output.getVar<byte>(i), component["members"][i]["value"], members[i]);

    return output;
}

std::vector<std::pair<AssetID, AssetType>> EditorAssemblyAsset::containedAssets() const
{
    std::vector<std::pair<AssetID, AssetType>> assets;
    assets.emplace_back(AssetID(_json["id"].asString()), AssetType::assembly);
    if(_json["linked"].asBool()) {
        for(auto& mesh : _json["linkedMeshes"])
            assets.emplace_back(AssetID(mesh["id"].asString()), AssetType::mesh);
    }
    return assets;
}

Asset* EditorAssemblyAsset::buildAssembly() const
{
    auto* assembly = new Assembly();
    assembly->name = name();
    assembly->id = _json["id"].asString();
    assembly->rootIndex = _json["rootEntity"].asUInt();

    for(auto& mesh : _json["dependencies"]["meshes"])
        assembly->meshes.emplace_back(mesh.asString());

    for(auto& material : _json["dependencies"]["materials"])
        assembly->materials.emplace_back(material.asString());

    std::unordered_set<const ComponentDescription*> components;
    for(auto& entity : _json["entities"]) {
        Assembly::EntityAsset entityAsset;
        if(entity.isMember("name")) {
            EntityName name;
            name.name = entity["name"].asString();
            entityAsset.components.emplace_back(name.toVirtual());
        }

        bool hasTransform = false;
        if(entity.isMember("children")) {
            Children cc;
            for(auto& c : entity["children"])
                cc.children.push_back({c.asUInt(), 0});
            entityAsset.components.emplace_back(cc.toVirtual());
            hasTransform = true;
        }

        for(auto& comp : entity["components"]) {
            if(comp["id"] == TRS::def()->asset->id.string())
                hasTransform = true;
            entityAsset.components.push_back(jsonToComponent(comp));
        }
        if(hasTransform) {
            Transform transform;
            transform.dirty = true;
            entityAsset.components.emplace_back(transform.toVirtual());
        }

        assembly->entities.push_back(entityAsset);
        for(auto& comp : entityAsset.components) {
            if(!components.count(comp.description()))
                components.insert(comp.description());
        }
    }
    for(auto* def : components)
        assembly->components.push_back(def->asset->id);

    return assembly;
}

Asset* EditorAssemblyAsset::buildMesh(const AssetID& id) const
{
    GLTFLoader gltf;
    if(!gltf.loadFromFile(_file.parent_path() / _json["source"].asString())) {
        Runtime::error("Could not load " + (_file.parent_path() / _json["source"].asString()).string());
        return nullptr;
    }

    Json::Value meshData = Json::nullValue;
    const Json::Value& meshes = _json["linkedMeshes"];
    for(Json::ArrayIndex index = 0; index < meshes.size(); ++index) {
        if(meshes[index]["id"] == id.string()) {
            meshData = gltf.json()["meshes"][index];
            break;
        }
    }
    if(meshData == Json::nullValue)
        return nullptr;

    auto* mesh = new MeshAsset();
    mesh->name = meshData["name"].asString();
    mesh->id = id;

    for(auto& primitive : meshData["primitives"]) {
        auto positions = gltf.readVec3Buffer(primitive["attributes"]["POSITION"].asUInt());
        size_t pIndex = mesh->addPrimitive(
            gltf.readScalarBuffer(primitive["indices"].asUInt()), static_cast<uint32_t>(positions.size()));
        mesh->addAttribute(pIndex, "POSITION", positions);

        if(primitive["attributes"].isMember("NORMAL")) {
            auto v = gltf.readVec3Buffer(primitive["attributes"]["NORMAL"].asUInt());
            mesh->addAttribute(pIndex, "NORMAL", v);
        }

        if(primitive["attributes"].isMember("TANGENT")) {
            // TODO account for tangents with bitangent sign stored as Vec4
            auto v = gltf.readVec4Buffer(primitive["attributes"]["TANGENT"].asUInt());
            mesh->addAttribute(pIndex, "TANGENT", v);
        }

        // TODO  make it so that we automatically detect all texcoords
        if(primitive["attributes"].isMember("TEXCOORD_0")) {
            auto v = gltf.readVec2Buffer(primitive["attributes"]["TEXCOORD_0"].asUInt());
            mesh->addAttribute(pIndex, "TEXCOORD_0", v);
        }

        // TODO: Remove vertices unused by indices array, since primitives reuse buffers
    }
    return mesh;
}

class CreateEntity : public JsonArrayChange {
  public:
    CreateEntity(Json::Value entity, VersionedJson* json)
        : JsonArrayChange("entities", (*json)["entities"].size(), std::move(entity), true, json){};

    void redo() override
    {
        JsonArrayChange::redo();

        uint32_t parentIndex = _value["parent"].asUInt();
        Json::insertArrayValue(_index, 0, _json->data()["entities"][parentIndex]["children"]);

        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly) {
            auto* arm = Runtime::getModule<AssemblyReloadManager>();
            arm->insertEntity(assembly, _index);
            arm->updateEntityParent(assembly, _index, parentIndex);
            for(auto& component : _value["components"])
                arm->addEntityComponent(assembly, _index, EditorAssemblyAsset::jsonToComponent(component));
        }
    }

    void undo() override
    {
        JsonArrayChange::undo();

        uint32_t parentIndex = _value["parent"].asUInt();
        Json::eraseArrayValue(0, _json->data()["entities"][parentIndex]["children"]);

        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly) {
            auto* arm = Runtime::getModule<AssemblyReloadManager>();
            arm->removeEntity(assembly, _index);
        }
    }
};

void EditorAssemblyAsset::createEntity(uint32_t parent)
{
    _json.tracker().recordChange(std::make_unique<CreateEntity>(newEntity(parent), &_json));
}

class DeleteEntity : public JsonChangeBase {
    uint32_t _entity;
    uint32_t _childIndex;
    std::map<uint32_t, Json::Value> _deletedEntities;

    uint32_t fixIndex(uint32_t index, bool increment)
    {
        uint32_t result = index;
        for(auto d : _deletedEntities) {
            if(d.first <= index)
                result += increment ? 1 : -1;
            else
                break;
        }
        return result;
    }

    void getDeletedIndices(uint32_t entityIndex, std::vector<uint32_t>& deleted)
    {

        auto& entity = _json->data()["entities"][entityIndex];
        for(auto& child : entity["children"])
            getDeletedIndices(child.asUInt(), deleted);
        deleted.push_back(entityIndex);
    }

  public:
    DeleteEntity(uint32_t entity, VersionedJson* json) : _entity(entity), JsonChangeBase(json)
    {
        std::vector<uint32_t> deletedIndices;
        getDeletedIndices(entity, deletedIndices);

        for(auto index : deletedIndices)
            _deletedEntities.insert({index, _json->data()["entities"][index]});
    }

    void redo() override
    {
        auto& json = _json->data();

        uint32_t parent = json["entities"][_entity]["parent"].asUInt();
        Json::Value newChildren;
        uint32_t childIndex = 0;
        for(auto& child : json["entities"][parent]["children"]) {
            if(child.asUInt() != _entity)
                newChildren.append(child);
            else
                _childIndex = childIndex;
            ++childIndex;
        }
        json["entities"][parent]["children"] = std::move(newChildren);

        Json::Value newEntities;
        uint32_t entityIndex = 0;
        for(auto& e : json["entities"]) {
            if(_deletedEntities.count(entityIndex++))
                continue;
            Json::Value entity = e;
            if(entity.isMember("parent") && entity["parent"].asUInt() > _entity)
                entity["parent"] = fixIndex(entity["parent"].asUInt(), false);

            if(entity.isMember("children")) {
                for(auto& child : entity["children"]) {
                    if(child.asUInt() > _entity)
                        child = fixIndex(child.asUInt(), false);
                }
            }
            newEntities.append(entity);
        }
        json["entities"] = std::move(newEntities);

        std::cout << "New entities after delete" << json["entities"] << std::endl;

        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{json["id"].asString()});
        if(assembly) {
            auto* arm = Runtime::getModule<AssemblyReloadManager>();
            for(auto e = _deletedEntities.rbegin(); e != _deletedEntities.rend(); ++e)
                arm->removeEntity(assembly, e->first);
        }

        Runtime::getModule<GUI>()->sendEvent<FocusEntityAssetEvent>(-1);
    }

    void undo() override
    {
        auto& json = _json->data();

        uint32_t parent = json["entities"][_entity]["parent"].asUInt();
        Json::Value newChildren;
        uint32_t childIndex = 0;
        for(auto& child : json["entities"][parent]["children"]) {
            if(childIndex == _childIndex)
                newChildren.append(_entity);
            newChildren.append(child);
            ++childIndex;
        }
        json["entities"][parent]["children"] = std::move(newChildren);

        Json::Value newEntities;
        uint32_t entityIndex = 0;
        for(auto& e : json["entities"]) {
            while(_deletedEntities.count(entityIndex)) {
                newEntities.append(_deletedEntities.at(entityIndex));
                ++entityIndex;
            }
            Json::Value entity = e;
            if(entity.isMember("parent") && entity["parent"].asUInt() > _entity)
                entity["parent"] = fixIndex(entity["parent"].asUInt(), true);

            if(entity.isMember("children")) {
                for(auto& child : entity["children"]) {
                    if(child.asUInt() > _entity)
                        child = fixIndex(child.asUInt(), true);
                }
            }
            newEntities.append(entity);
            ++entityIndex;
        }
        while(_deletedEntities.count(entityIndex)) {
            newEntities.append(_deletedEntities.at(entityIndex));
            ++entityIndex;
        }
        json["entities"] = std::move(newEntities);
        std::cout << "New entities after reversed delete" << json["entities"] << std::endl;

        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{json["id"].asString()});
        if(assembly) {
            auto* arm = Runtime::getModule<AssemblyReloadManager>();
            for(auto& e : _deletedEntities) {
                arm->insertEntity(assembly, e.first);
                for(auto& component : e.second["components"])
                    arm->addEntityComponent(assembly, e.first, EditorAssemblyAsset::jsonToComponent(component));
            }
            for(auto& e : _deletedEntities) {
                arm->updateEntityParent(assembly, e.first, e.second["parent"].asUInt());
            }
        }

        Runtime::getModule<GUI>()->sendEvent<FocusEntityAssetEvent>(-1);
    }
};

void EditorAssemblyAsset::deleteEntity(uint32_t entity)
{
    _json.recordChange(std::make_unique<DeleteEntity>(entity, &_json));
}

class ParentEntity : public JsonChangeBase {
    uint32_t _entity;
    uint32_t _oldIndex;
    uint32_t _newIndex;
    uint32_t _oldParent;
    uint32_t _newParent;

    uint32_t changeParent(uint32_t entity, uint32_t parent, uint32_t index)
    {
        auto& json = _json->data();
        uint32_t oldParent = json["entities"][entity]["parent"].asUInt();
        json["entities"][entity]["parent"] = parent;

        Json::Value newChildren;
        uint32_t oldIndex = 0;
        uint32_t cIndex = 0;
        for(auto& child : json["entities"][oldParent]["children"]) {
            if(child.asUInt() != entity)
                newChildren.append(child);
            else
                oldIndex = cIndex;
            ++cIndex;
        }
        if(oldParent == parent && oldIndex < index)
            --index;
        json["entities"][oldParent]["children"] = std::move(newChildren);
        Json::insertArrayValue(entity, index, json["entities"][parent]["children"]);

        return oldIndex;
    }

  public:
    ParentEntity(uint32_t entity, uint32_t parent, uint32_t index, VersionedJson* json) : JsonChangeBase(json)
    {
        _entity = entity;
        _oldParent = json->data()["entities"][entity]["parent"].asUInt();
        _newParent = parent;
        _newIndex = index;
    };

    void redo() override
    {
        _oldIndex = changeParent(_entity, _newParent, _newIndex);
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->updateEntityParent(assembly, _entity, _newParent);
    }

    void undo() override
    {
        changeParent(_entity, _oldParent, _oldIndex);
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->updateEntityParent(assembly, _entity, _oldParent);
    }
};

void EditorAssemblyAsset::parentEntity(uint32_t entity, uint32_t parent, uint32_t index)
{
    _json.recordChange(std::make_unique<ParentEntity>(entity, parent, index, &_json));
}

class UpdateEntityComponent : public JsonChange {
    uint32_t _entity;

  public:
    UpdateEntityComponent(
        uint32_t entity,
        uint32_t componentIndex,
        Json::Value componentBefore,
        Json::Value component,
        VersionedJson* json)
        : JsonChange(
              "entities/" + std::to_string(entity) + "/components/" + std::to_string(componentIndex),
              std::move(componentBefore),
              std::move(component),
              json)
    {
        _entity = entity;
    }

    void redo() override
    {
        JsonChange::redo();
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->updateEntityComponent(
                assembly, _entity, EditorAssemblyAsset::jsonToComponent(_after));
    }

    void undo() override
    {
        assert(!_before.isNull());
        JsonChange::undo();
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->updateEntityComponent(
                assembly, _entity, EditorAssemblyAsset::jsonToComponent(_before));
    }
};

void EditorAssemblyAsset::updateEntityComponent(uint32_t entity, VirtualComponentView component, bool continuous)
{
    uint32_t componentIndex = 0;
    for(auto& c : _json["entities"][entity]["components"]) {
        if(c["id"].asString() == component.description()->asset->id.string())
            break;
        ++componentIndex;
    }
    if(componentIndex == _json["entities"][entity]["components"].size())
        return; // Component was not found

    auto newComponent = componentToJson(component);
    if(continuous) {
        if(_componentBefore.isNull())
            _componentBefore = newComponent;

        _json.data()["entities"][entity]["components"][componentIndex] = newComponent;
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{_json["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->updateEntityComponent(assembly, entity, component);
    }
    else {
        _json.recordChange(
            std::make_unique<UpdateEntityComponent>(entity, componentIndex, _componentBefore, newComponent, &_json));
        _componentBefore = Json::nullValue;
    }
}

void EditorAssemblyAsset::updateEntityComponent(uint32_t entity, uint32_t component, Json::Value value, bool continuous)
{
    if(continuous) {
        if(_componentBefore.isNull())
            _componentBefore = value;
        _json.data()["entities"][entity]["components"][component] = value;
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{_json["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->updateEntityComponent(
                assembly, entity, EditorAssemblyAsset::jsonToComponent(value));
    }
    else {
        _json.recordChange(
            std::make_unique<UpdateEntityComponent>(entity, component, _componentBefore, std::move(value), &_json));
        _componentBefore = Json::nullValue;
    }
}

class AddEntityComponent : public JsonArrayChange {
    uint32_t _entity;

  public:
    AddEntityComponent(uint32_t entity, Json::Value component, VersionedJson* json)
        : JsonArrayChange("entities/" + std::to_string(entity) + "/components", 0, std::move(component), true, json)
    {
        _entity = entity;
    }

    void redo() override
    {
        JsonArrayChange::redo();
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->addEntityComponent(
                assembly, _entity, EditorAssemblyAsset::jsonToComponent(_value));
    }

    void undo() override
    {
        JsonArrayChange::undo();
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->removeEntityComponent(
                assembly, _entity, EditorAssemblyAsset::jsonToComponent(_value).description()->id);
    }
};

void EditorAssemblyAsset::addEntityComponent(uint32_t entity, Json::Value component)
{
    _json.recordChange(std::make_unique<AddEntityComponent>(entity, std::move(component), &_json));
}

class RemoveEntityComponent : public JsonArrayChange {
    uint32_t _entity;

  public:
    RemoveEntityComponent(uint32_t entity, uint32_t componentIndex, VersionedJson* json)
        : JsonArrayChange(
              "entities/" + std::to_string(entity) + "/components", componentIndex, Json::nullValue, false, json)
    {
        _entity = entity;
    }

    void redo() override
    {
        JsonArrayChange::redo();
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->removeEntityComponent(
                assembly, _entity, EditorAssemblyAsset::jsonToComponent(_value).description()->id);
    }

    void undo() override
    {
        JsonArrayChange::undo();
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(assembly)
            Runtime::getModule<AssemblyReloadManager>()->addEntityComponent(
                assembly, _entity, EditorAssemblyAsset::jsonToComponent(_value));
    }
};

void EditorAssemblyAsset::removeEntityComponent(uint32_t entity, uint32_t component)
{
    _json.recordChange(std::make_unique<RemoveEntityComponent>(entity, component, &_json));
}

class MaterialChange : public JsonChange {
    uint32_t _materialIndex;

    static Json::Value generateAfter(uint32_t materialIndex, const AssetID& materialID, VersionedJson* json)
    {
        Json::Value& materials = json->data()["dependencies"]["materials"];
        while(materialIndex >= materials.size())
            materials.append("null");

        return materialID.string();
    }

    void updateMaterialRenderers()
    {
        auto* am = Runtime::getModule<AssetManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID{(*_json)["id"].asString()});
        if(!assembly)
            return;
        auto& materials = (*_json)["dependencies"]["materials"];
        if(assembly->materials.size() != materials.size())
            assembly->materials.resize(materials.size());
        assembly->materials[_materialIndex] = AssetID(materials[_materialIndex].asString());

        auto* arm = Runtime::getModule<AssemblyReloadManager>();
        uint32_t entityIndex = 0;
        for(auto& entity : _json->data()["entities"]) {
            for(auto& component : entity["components"]) {
                if(component["name"] == MeshRendererComponent::def()->name)
                    arm->updateEntityComponent(assembly, entityIndex, EditorAssemblyAsset::jsonToComponent(component));
            }
            ++entityIndex;
        }
    }

  public:
    MaterialChange(uint32_t materialIndex, const AssetID& materialID, VersionedJson* json)
        : JsonChange(
              "dependencies/materials/" + std::to_string(materialIndex),
              generateAfter(materialIndex, materialID, json),
              json),
          _materialIndex(materialIndex){};

    void redo() override
    {
        JsonChange::redo();
        updateMaterialRenderers();
    }

    void undo() override
    {
        JsonChange::undo();
        updateMaterialRenderers();
    }
};

void EditorAssemblyAsset::changeMaterial(uint32_t materialIndex, const AssetID& materialID)
{
    if(materialID.null())
        _json.recordChange(std::make_unique<MaterialChange>(materialIndex, materialID, &_json));
    else
        Runtime::getModule<AssetManager>()
            ->fetchAsset<Asset>(materialID)
            .then([this, materialIndex, materialID](Asset* m) {
                _json.recordChange(std::make_unique<MaterialChange>(materialIndex, materialID, &_json));
            });
}
