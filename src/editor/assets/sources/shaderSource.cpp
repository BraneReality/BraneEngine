#include "shaderSource.h"
#include "editor/editor.h"
#include "fileManager/fileManager.h"

ShaderAssetSource::ShaderAssetSource(std::filesystem::path path) : AssetSource(path)
{
    setSaved();
    updateAttributes();
}

void ShaderAssetSource::updateAttributes()
{
    std::string shaderCode;
    if(!FileManager::readFile(path, shaderCode))
    {
        Runtime::error("Failed to open " + path.string());
        return;
    }
    auto editor = Runtime::getModule<Editor>();
    auto& compiler = editor->shaderCompiler();
    auto includer = std::make_unique<ShaderIncluder>();
    std::filesystem::path dir = std::filesystem::path{path}.remove_filename();
    includer->addSearchDir(dir);
    compiler.extractAttributes(shaderCode, shaderType(), std::move(includer), attributes);
}

std::vector<std::pair<AssetSourceID, AssetType>> ShaderAssetSource::exportedAssets() const
{
    return {{AssetSourceID{Json::Value("main")}, AssetType::shader}};
}

Result<std::shared_ptr<AssetMetadata>> ShaderAssetSource::buildMetadata(const AssetSourceID& id) const
{
    if(id.data != "main")
        return Err(std::string("ShaderAssetSource only exports 'main'"));
    return Ok<std::shared_ptr<AssetMetadata>>(std::make_shared<ShaderAssetMetadata>());
}

Result<std::shared_ptr<Asset>> ShaderAssetSource::buildAsset(std::shared_ptr<AssetMetadata> metadata) const
{
    auto imd = std::dynamic_pointer_cast<ShaderAssetMetadata>(metadata);
    if(!imd)
        return Err((std::string) "Incorrect metadata passed");

    std::string fileSuffix = path.extension().string();

    auto shader = std::make_shared<ShaderAsset>();
    shader->id = *metadata->exportId->value();
    shader->name = path.stem().string();
    shader->shaderType = shaderType();

    std::string shaderCode;
    if(!FileManager::readFile(path, shaderCode))
        return Err("Failed to open shader source: " + path.string());
    auto editor = Runtime::getModule<Editor>();
    auto& compiler = editor->shaderCompiler();

    auto includer = std::make_unique<ShaderIncluder>();
    std::filesystem::path dir = std::filesystem::path{path}.remove_filename();
    includer->addSearchDir(dir);
    if(!compiler.compileShader(shaderCode, shader->shaderType, shader->spirv, std::move(includer)))
        return Err<std::string>("Failed to compile shader");

    includer = std::make_unique<ShaderIncluder>();
    includer->addSearchDir(dir);

    ShaderCompiler::ShaderAttributes attributes;
    compiler.extractAttributes(shaderCode, shader->shaderType, std::move(includer), attributes);
    for(auto& u : attributes.uniforms)
        shader->uniforms.insert({u.name, u});

    shader->inputs = std::move(attributes.inputVariables);
    shader->outputs = std::move(attributes.outputVariables);

    return Ok<std::shared_ptr<Asset>>(shader);
}

Result<void> ShaderAssetSource::save()
{
    return Ok<void>();
}

AssetSourceType ShaderAssetSource::type()
{
    return std::static_pointer_cast<ShaderAssetSource>(shared_from_this());
}

ShaderType ShaderAssetSource::shaderType() const
{
    auto ext = path.extension();
    if(ext == ".vert")
        return ShaderType::vertex;
    else if(ext == ".frag")
        return ShaderType::fragment;
    else if(ext == ".comp")
        return ShaderType::compute;
    Runtime::error("Unknown shader file extension: " + ext.string());
    return ShaderType::compute;
}

ShaderAssetMetadata::ShaderAssetMetadata()
    : AssetMetadata({Json::Value("main")}, BraneAssetID{"${default}", UUID::generate().ok()})
{}

std::string ShaderAssetMetadata::typeName() const
{
    return "Shader";
}

Result<Json::Value> ShaderAssetMetadata::serialize() const
{
    Json::Value value = Json::Value(Json::ValueType::objectValue);
    auto res = JsonParseUtil::write<AssetMetadata>(value, *this);
    if(!res)
        return Err(res.err().toString());
    return Ok<Json::Value>(value);
}

AssetMetadataType ShaderAssetMetadata::type()
{
    return AssetMetadataType(std::static_pointer_cast<ShaderAssetMetadata>(shared_from_this()));
}

AssetType ShaderAssetMetadata::assetType() const
{
    return AssetType::shader;
}
