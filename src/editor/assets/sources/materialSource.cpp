#include "materialSource.h"
#include "assets/assetManager.h"
#include "assets/types/materialAsset.h"
#include "assets/types/shaderAsset.h"
#include "fileManager/fileManager.h"

MaterialAssetSource::MaterialAssetSource(std::filesystem::path path)
    : AssetSource(path), vertexShader(std::make_shared<TrackedValue<AssetID>>()),
      fragmentShader(std::make_shared<TrackedValue<AssetID>>()),
      properties(std::make_shared<TrackedVector<TrackedValue<PropVar>>>()),
      textureBindings(std::make_shared<TrackedVector<TrackedValue<TextureBinding>>>())
{
    setSaved();
    validateProperties();
}

void MaterialAssetSource::initMembers(Option<std::shared_ptr<TrackedType>> parent)
{
    AssetSource::initMembers(parent);
    auto p = Some(shared_from_this());
    vertexShader->initMembers(p);
    fragmentShader->initMembers(p);
    properties->initMembers(p);
    textureBindings->initMembers(p);
}

std::vector<std::pair<AssetSourceID, AssetType>> MaterialAssetSource::exportedAssets() const
{
    return {{AssetSourceID{Json::Value("main")}, AssetType::material}};
}

Result<std::shared_ptr<AssetMetadata>> MaterialAssetSource::buildMetadata(const AssetSourceID& id) const
{
    if(id.data != "main")
        return Err(std::string("MaterialAssetSource only exports 'main'"));
    return Ok<std::shared_ptr<AssetMetadata>>(std::make_shared<MaterialAssetMetadata>());
}

Result<std::shared_ptr<Asset>> MaterialAssetSource::buildAsset(std::shared_ptr<AssetMetadata> metadata) const
{
    auto imd = std::dynamic_pointer_cast<MaterialAssetMetadata>(metadata);
    if(!imd)
        return Err((std::string) "Incorrect metadata passed");

    std::string fileSuffix = path.extension().string();

    auto material = std::make_shared<MaterialAsset>();
    material->name = path.stem().string();
    material->id = *metadata->exportId->value();
    material->vertexShader = *fragmentShader->value();
    material->fragmentShader = *fragmentShader->value();
    for(auto& texture : *textureBindings->values())
    {
        auto value = texture->value();
        std::pair<uint16_t, AssetID> tb = {value->binding, value->id};
        material->textures.push_back(tb);
    }
    material->serializedProperties = serializeProperties();

    return Ok<std::shared_ptr<Asset>>(material);
}

Result<void> MaterialAssetSource::save()
{
    if(!unsavedChanges())
        return Ok<void>();

    Json::Value json;
    auto res = JsonSerializer<MaterialAssetSource>::write(json, *this);
    if(!res)
        return Err(res.err().toString());
    FileManager::writeFile(path, json);
    // Err(std::format("Failed to write material to {}", path.string()));
    return Ok<void>();
}

AssetSourceType MaterialAssetSource::type()
{
    return AssetSourceType(std::static_pointer_cast<MaterialAssetSource>(shared_from_this()));
}

size_t alignedTo(size_t index, size_t alignment)
{
    size_t al = alignment - index % alignment;
    if(al == alignment)
        return index;
    return index + al;
}

template<class T>
void alignedPush(std::vector<uint8_t>& data, T value)
{
    static_assert(std::is_trivially_copyable<T>());
    data.resize(alignedTo(data.size(), sizeof(T)));
    size_t index = data.size() - sizeof(T);
    *(T*)(data.data() + index) = value;
}

AsyncData<bool> MaterialAssetSource::validateProperties()
{
    AsyncData<bool> result;
    if(fragmentShader->value()->empty())
    {
        properties->clear().now();
        textureBindings->clear().now();
        result.setData(true);
        return result;
    }

    Runtime::getModule<AssetManager>()
        ->fetchAsset<ShaderAsset>(*fragmentShader->value())
        .then([this, result](Shared<ShaderAsset> fragmentShader) {
        auto uniform = fragmentShader->uniforms.find("MaterialProperties");
        if(uniform == fragmentShader->uniforms.end())
        {
            properties->clear().now();
            return;
        }

        std::unordered_map<std::string, Shared<TrackedValue<PropVar>>> oldValues;
        for(auto& var : *properties->values())
            oldValues.insert({*var->value()->name, var});
        properties->clear().now();

        auto& members = uniform->second.members;
        for(auto& member : members)
        {
            const std::string& name = member.name;
            PropVar::ValueType defaultValue;

            switch(member.layout())
            {
                case ShaderVariableData::scalar:
                {
                    switch(member.type)
                    {
                        case ShaderVariableData::Boolean:
                            defaultValue = false;
                            break;
                        case ShaderVariableData::Int:
                            defaultValue = (int)0;
                            break;
                        case ShaderVariableData::Float:
                            defaultValue = (float)0;
                            break;
                        default:
                            Runtime::error("MaterialProperties does not support " +
                                           member.typeNames.toString(member.type) + " types");
                            return;
                    }
                }
                break;
                case ShaderVariableData::vec2:
                    defaultValue = glm::vec2();
                    break;
                case ShaderVariableData::vec3:
                    defaultValue = glm::vec3();
                    break;
                case ShaderVariableData::vec4:
                    defaultValue = glm::vec4();
                    break;
                default:
                    Runtime::error("MaterialProperties does not support " +
                                   member.layoutNames.toString(member.layout()) + " yet");
                    return;
            }
            if(oldValues.contains(name) && oldValues[name]->value()->type == member.type)
                properties->push_back(oldValues[name]).now();
            else
            {
                properties
                    ->push_back(std::make_shared<TrackedValue<PropVar>>(PropVar{
                        .name = std::make_shared<std::string>(name),
                        .value = defaultValue,
                        .type = member.type,
                    }))
                    .now();
            }
        }

        std::unordered_map<std::string, Shared<TrackedValue<TextureBinding>>> oldTextureBindings;
        for(auto tb : *textureBindings->values())
            oldTextureBindings.insert({*tb->value()->name, tb});
        textureBindings->clear().now();
        for(auto& binding : fragmentShader->samplers)
        {
            if(oldTextureBindings.contains(binding.first))
                textureBindings->push_back(oldTextureBindings[binding.first]).now();
            else
                textureBindings
                    ->push_back(std::make_shared<TrackedValue<TextureBinding>>(
                        TextureBinding{.name = std::make_shared<std::string>(binding.second.name),
                                       .id = AssetID(),
                                       .binding = binding.second.binding}))
                    .now();
        }

        result.setData(true);
    }).onError([this, result](std::string error) {
        result.setError(error);
        Runtime::error(std::format("Failed to load fragmentShader for material {}: ", path.string(), error));
    });
    return result;
}

std::vector<uint8_t> MaterialAssetSource::serializeProperties() const
{
    std::vector<uint8_t> props;
    for(auto& prop : *properties->values())
    {
        MATCHV(prop->value()->value, [&](auto value) { alignedPush(props, value); });
    }
    return props;
}

MaterialAssetMetadata::MaterialAssetMetadata()
    : AssetMetadata({Json::Value("main")}, BraneAssetID{"${default}", UUID::generate().ok()})
{}

std::string MaterialAssetMetadata::typeName() const
{
    return "Material";
}

Result<Json::Value> MaterialAssetMetadata::serialize() const
{
    Json::Value value = Json::Value(Json::ValueType::objectValue);
    auto res = JsonParseUtil::write<AssetMetadata>(value, *this);
    if(!res)
        return Err(res.err().toString());
    return Ok<Json::Value>(value);
}

AssetMetadataType MaterialAssetMetadata::type()
{
    return AssetMetadataType(std::static_pointer_cast<MaterialAssetMetadata>(shared_from_this()));
}

AssetType MaterialAssetMetadata::assetType() const
{
    return AssetType::material;
}
