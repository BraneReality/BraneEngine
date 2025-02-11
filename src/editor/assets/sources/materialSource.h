#pragma once
#include <filesystem>
#include "assets/types/shaderAsset.h"
#include "editor/assets/editorAsset.h"
#include "editor/state/trackedVector.h"
#include <glm/glm.hpp>

class ShaderAsset;

struct MaterialAssetSource : public AssetSource
{
    Shared<TrackedValue<AssetID>> vertexShader;
    Shared<TrackedValue<AssetID>> fragmentShader;

    struct PropVar
    {
        Shared<std::string> name =
            nullptr; // put this in a shared since it won't change due to use changes, and it gets copied a lot
        using ValueType = std::variant<bool, int, float, glm::vec2, glm::vec3, glm::vec4>;
        ValueType value = false;
        ShaderVariableData::Type type = ShaderVariableData::Boolean;
    };

    Shared<TrackedVector<TrackedValue<PropVar>>> vertexShaderProperties;

    struct TextureBinding
    {
        AssetID id;
        uint16_t binding;
    };

    Shared<TrackedVector<TrackedValue<TextureBinding>>> textureBindings;

    MaterialAssetSource(std::filesystem::path path);
    void initMembers(Option<std::shared_ptr<TrackedType>> parent) override;
    std::vector<std::pair<AssetSourceID, AssetType>> exportedAssets() const override;
    Result<std::shared_ptr<AssetMetadata>> buildMetadata(const AssetSourceID& id) const override;
    Result<std::shared_ptr<Asset>> buildAsset(std::shared_ptr<AssetMetadata> metadata) const override;
    Result<void> save() override;
    AssetSourceType type() override;

    void validateProperties(std::shared_ptr<ShaderAsset> vertexShader);
    std::vector<uint8_t> serializeProperties() const;
};

struct MaterialAssetMetadata : public AssetMetadata
{
    MaterialAssetMetadata();
    std::string typeName() const override;
    Result<Json::Value> serialize() const override;
    AssetMetadataType type() override;
    AssetType assetType() const override;
};

template<>
struct JsonSerializer<MaterialAssetSource::PropVar>
{
    static Result<void, JsonSerializerError> read(const Json::Value& json, MaterialAssetSource::PropVar& value)
    {
        CHECK_RESULT(JsonParseUtil::read(json["name"], value.name));
        CHECK_RESULT(JsonParseUtil::read(json["value"], value.value));

        std::string typeName;
        CHECK_RESULT(JsonParseUtil::read(json["type"], typeName));
        value.type = ShaderVariableData::typeNames.toEnum(typeName);
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const MaterialAssetSource::PropVar& value)
    {
        CHECK_RESULT(JsonParseUtil::write(json["name"], value.name));
        CHECK_RESULT(JsonParseUtil::write(json["value"], value.value));

        CHECK_RESULT(JsonParseUtil::write(json["type"], ShaderVariableData::typeNames.toString(value.type)));
        return Ok<void>();
    }
};

template<>
struct JsonSerializer<MaterialAssetSource>

{
    static Result<void, JsonSerializerError> read(const Json::Value& json, MaterialAssetSource& value)
    {
        auto members = {"vertexShader", "fragmentShader", "vertexShaderProperties", "textureBindings"};
        for(auto& m : members)
        {
            if(!json.isMember(m))
                return Err(JsonSerializerError(JsonSerializerError::MissingKey, std::format("Expecting member {}", m)));
        }

        CHECK_RESULT(JsonParseUtil::read(json["vertexShader"], value.vertexShader));
        CHECK_RESULT(JsonParseUtil::read(json["fragmentShader"], value.fragmentShader));
        CHECK_RESULT(JsonParseUtil::read(json["vertexShaderProperties"], value.vertexShaderProperties));
        CHECK_RESULT(JsonParseUtil::read(json["textureBindings"], value.textureBindings));
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const MaterialAssetSource& value)
    {
        CHECK_RESULT(JsonParseUtil::write(json["vertexShader"], value.vertexShader));
        CHECK_RESULT(JsonParseUtil::write(json["fragmentShader"], value.fragmentShader));
        CHECK_RESULT(JsonParseUtil::write(json["vertexShaderProperties"], value.vertexShaderProperties));
        CHECK_RESULT(JsonParseUtil::write(json["textureBindings"], value.textureBindings));
        return Ok<void>();
    }
};
