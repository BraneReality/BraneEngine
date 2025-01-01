//
// Created by eli on 8/25/2022.
//

#include "editorMaterialAsset.h"
#include "assets/assetManager.h"
#include "assets/types/materialAsset.h"
#include "editorShaderAsset.h"
#include "graphics/graphics.h"
#include "graphics/material.h"
#include "utility/jsonTypeUtilities.h"

EditorMaterialAsset::EditorMaterialAsset(const std::filesystem::path& file, BraneProject& project)
    : EditorAsset(file, project)
{
    // Generate default
    if(!std::filesystem::exists(_file))
    {
        _json.data()["inputs"] = Json::arrayValue;
        _json.data()["vertexShader"] = "null"; // TODO default fragment and vertex shaders applied here
        _json.data()["fragmentShader"] = "null";
    }
}

Asset* EditorMaterialAsset::buildAsset(const AssetID& id) const
{
    assert(id.toString() == _json["id"].asString());
    auto* material = new MaterialAsset();
    material->name = name();
    material->id = AssetID::parse(_json["id"].asString()).ok();
    material->vertexShader = AssetID::parse(_json["vertexShader"].asString()).ok();
    material->fragmentShader = AssetID::parse(_json["fragmentShader"].asString()).ok();
    for(auto& texture : _json["textures"])
    {
        std::pair<uint16_t, AssetID> tb;
        tb.first = texture["binding"].asUInt();
        tb.second = AssetID::parse(texture["id"].asString()).ok();
        material->textures.push_back(tb);
    }
    material->serializedProperties = serializeProperties();

    return material;
}

std::vector<std::pair<AssetID, AssetType>> EditorMaterialAsset::containedAssets() const
{
    std::vector<std::pair<AssetID, AssetType>> deps;
    deps.emplace_back(AssetID::parse(_json["id"].asString()).ok(), AssetType::material);
    return std::move(deps);
}

size_t alignedTo(size_t index, size_t alignment)
{
    size_t al = alignment - index % alignment;
    if(al == alignment)
        return index;
    return index + al;
}

std::vector<uint8_t> EditorMaterialAsset::serializeProperties() const
{
    std::vector<uint8_t> sp;
    for(auto& prop : _json["properties"])
    {
        std::string type = prop["type"].asString();
        if(type == "bool")
        {
            sp.resize(alignedTo(sp.size(), sizeof(bool)));
            sp.push_back(prop["value"].asBool());
        }
        else if(type == "int")
        {
            sp.resize(alignedTo(sp.size(), sizeof(int)) + sizeof(int));
            size_t index = sp.size() - sizeof(int);
            *(int*)&sp[index] = prop["value"].asInt();
        }
        else if(type == "float")
        {
            sp.resize(alignedTo(sp.size(), sizeof(float)) + sizeof(float));
            size_t index = sp.size() - sizeof(float);
            *(float*)&sp[index] = prop["value"].asFloat();
        }
        else if(type == "vec2")
        {
            sp.resize(alignedTo(sp.size(), sizeof(glm::vec2)) + sizeof(glm::vec2));
            size_t index = sp.size() - sizeof(glm::vec2);
            *(glm::vec2*)&sp[index] = fromJson<glm::vec2>(prop["value"]);
        }
        else if(type == "vec3")
        {
            sp.resize(alignedTo(sp.size(), sizeof(glm::vec4)) + sizeof(glm::vec3));
            size_t index = sp.size() - sizeof(glm::vec3);
            *(glm::vec3*)&sp[index] = fromJson<glm::vec3>(prop["value"]);
        }
        else if(type == "vec4")
        {
            sp.resize(alignedTo(sp.size(), sizeof(glm::vec4)) + sizeof(glm::vec4));
            size_t index = sp.size() - sizeof(glm::vec4);
            *(glm::vec4*)&sp[index] = fromJson<glm::vec4>(prop["value"]);
        }
    }
    return std::move(sp);
}

void EditorMaterialAsset::initializeProperties(EditorShaderAsset* shaderAsset)
{
    auto& props = _json.data()["properties"];
    if(!shaderAsset->json()["attributes"]["uniforms"].isMember("MaterialProperties"))
    {
        props.clear();
        return;
    }
    std::unordered_map<std::string, Json::Value> oldValues;
    for(auto& prop : props)
        oldValues.insert({prop["name"].asString(), prop["value"]});
    props.clear();

    for(auto& member : shaderAsset->json()["attributes"]["uniforms"]["MaterialProperties"]["members"])
    {
        Json::Value newProp;
        std::string name = member["name"].asString();
        newProp["name"] = name;
        auto layout = ShaderVariableData::layoutNames.toEnum(member["layout"].asString());
        switch(layout)
        {

            case ShaderVariableData::scalar:
            {
                auto type = ShaderVariableData::typeNames.toEnum(member["type"].asString());
                switch(type)
                {
                    case ShaderVariableData::Boolean:
                        newProp["type"] = "bool";
                        newProp["value"] = false;
                        break;
                    case ShaderVariableData::Int:
                        newProp["type"] = "int";
                        newProp["value"] = 0;
                        break;
                    case ShaderVariableData::Float:
                        newProp["type"] = "float";
                        newProp["value"] = 0;
                        break;
                    default:
                        Runtime::error("MaterialProperties does not support " + member["type"].asString() + " types");
                        return;
                }
            }
            break;
            case ShaderVariableData::vec2:
                newProp["type"] = "vec2";
                newProp["value"].append(0);
                newProp["value"].append(0);
                break;
            case ShaderVariableData::vec3:
                newProp["type"] = "vec3";
                newProp["value"].append(0);
                newProp["value"].append(0);
                newProp["value"].append(0);
                break;
            case ShaderVariableData::vec4:
                newProp["type"] = "vec4";
                newProp["value"].append(0);
                newProp["value"].append(0);
                newProp["value"].append(0);
                newProp["value"].append(0);
                break;
            default:
                Runtime::error("MaterialProperties does not support " + member["layout"].asString() + " yet");
                return;
        }
        if(oldValues.count(name) && oldValues[name].type() == newProp["value"].type())
            newProp["value"] = oldValues[name];
        props.append(newProp);
    }
}

class PropertyChange : public JsonChange
{
    EditorMaterialAsset* _asset;

  public:
    PropertyChange(size_t index, Json::Value value, Json::Value before, EditorMaterialAsset* asset, VersionedJson* json)
        : JsonChange("properties/" + std::to_string(index) + "/value", std::move(before), std::move(value), json)
    {
        _asset = asset;
    }

    void updateProps()
    {
        auto am = Runtime::getModule<AssetManager>();
        auto asset = am->getAsset<MaterialAsset>(_asset->id());
        if(!asset)
            return;
        graphics::Material* material = Runtime::getModule<graphics::VulkanRuntime>()->getMaterial(asset->runtimeID);
        material->setMaterialProperties(_asset->serializeProperties());
    }

    void redo() override
    {
        JsonChange::redo();
        updateProps();
    }

    void undo() override
    {
        JsonChange::undo();
        updateProps();
    }
};

Json::Value _sketchyStateholdingVariableBecauseImLazy;

void EditorMaterialAsset::changeProperty(size_t index, Json::Value value, bool finished)
{
    if(finished)
    {
        _json.recordChange(std::make_unique<PropertyChange>(
            index, std::move(value), _sketchyStateholdingVariableBecauseImLazy, this, &_json));
        _sketchyStateholdingVariableBecauseImLazy = Json::nullValue;
        return;
    }
    /* Preview change */
    auto& jsonValue = Json::resolvePath("properties/" + std::to_string(index) + "/value", _json.data());
    if(_sketchyStateholdingVariableBecauseImLazy.isNull())
        _sketchyStateholdingVariableBecauseImLazy = jsonValue;
    jsonValue = value;

    auto am = Runtime::getModule<AssetManager>();
    auto asset = am->getAsset<MaterialAsset>(id());
    if(!asset)
        return;
    graphics::Material* material = Runtime::getModule<graphics::VulkanRuntime>()->getMaterial(asset->runtimeID);
    material->setMaterialProperties(serializeProperties());
}
