//
// Created by eli on 8/14/2022.
//

#include "braneProject.h"
#include <atomic>
#include <filesystem>
#include <utility>
#include "assets/assetManager.h"
#include "assets/editorAsset.h"
#include "editor.h"
#include "editor/assets/sources/materialSource.h"
#include "fileManager/fileManager.h"
#include "runtime/runtime.h"
#include <condition_variable>

BraneAssetServerInfo::BraneAssetServerInfo()
    : address(std::make_shared<TrackedValue<std::string>>("localhost")),
      port(std::make_shared<TrackedValue<uint32_t>>(2001)) {};

void BraneAssetServerInfo::initMembers(Option<std::shared_ptr<TrackedType>> parent)
{
    TrackedObject::initMembers(parent);
    auto p = Some(shared_from_this());
    address->initMembers(p);
    port->initMembers(p);
}

BraneGraphicsSettings::BraneGraphicsSettings()
    : defaultVertexShader(std::make_shared<TrackedValue<AssetID>>()),
      defaultFragmentShader(std::make_shared<TrackedValue<AssetID>>()),
      defaultMaterial(std::make_shared<TrackedValue<AssetID>>()) {};

void BraneGraphicsSettings::initMembers(Option<std::shared_ptr<TrackedType>> parent)
{
    TrackedObject::initMembers(parent);
    auto p = Some(shared_from_this());
    defaultVertexShader->initMembers(p);
    defaultFragmentShader->initMembers(p);
    defaultMaterial->initMembers(p);
}

BraneProjectData::BraneProjectData()
    : name(std::make_shared<TrackedValue<std::string>>("New project")),
      assetServer(std::make_shared<BraneAssetServerInfo>()), graphics(std::make_shared<BraneGraphicsSettings>())
{}

void BraneProjectData::initMembers(Option<std::shared_ptr<TrackedType>> parent)
{
    TrackedType::initMembers(parent);
    auto p = Some(shared_from_this());
    name->initMembers(p);
    assetServer->initMembers(p);
}

BraneProject::BraneProject(std::filesystem::path root)
    : _root(root), _indexer(std::make_shared<AssetIndexer>()), _data(std::make_shared<BraneProjectData>())
{
    _data->initMembers(None());
}

BraneProject::~BraneProject() {}

template<>
struct JsonSerializer<BraneAssetServerInfo>
{
    static Result<void, JsonSerializerError> read(const Json::Value& json, BraneAssetServerInfo& data)
    {
        CHECK_RESULT(JsonParseUtil::read(json["address"], data.address));
        CHECK_RESULT(JsonParseUtil::read(json["port"], data.port));
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const BraneAssetServerInfo& data)
    {
        CHECK_RESULT(JsonParseUtil::write(json["address"], data.address));
        CHECK_RESULT(JsonParseUtil::write(json["port"], data.port));
        return Ok<void>();
    }
};

template<>
struct JsonSerializer<BraneGraphicsSettings>
{
    static Result<void, JsonSerializerError> read(const Json::Value& json, BraneGraphicsSettings& data)
    {

        if(json.isMember("defaultVertexShader"))
            CHECK_RESULT(JsonParseUtil::read(json["defaultVertexShader"], data.defaultVertexShader));
        if(json.isMember("defaultFragmentShader"))
            CHECK_RESULT(JsonParseUtil::read(json["defaultFragmentShader"], data.defaultFragmentShader));
        if(json.isMember("defaultMaterial"))
            CHECK_RESULT(JsonParseUtil::read(json["defaultMaterial"], data.defaultMaterial));
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const BraneGraphicsSettings& data)
    {
        CHECK_RESULT(JsonParseUtil::write(json["defaultVertexShader"], data.defaultVertexShader));
        CHECK_RESULT(JsonParseUtil::write(json["defaultFragmentShader"], data.defaultFragmentShader));
        CHECK_RESULT(JsonParseUtil::write(json["defaultMaterial"], data.defaultMaterial));
        return Ok<void>();
    }
};

template<>
struct JsonSerializer<BraneProjectData>
{
    static Result<void, JsonSerializerError> read(const Json::Value& json, BraneProjectData& data)
    {

        CHECK_RESULT(JsonParseUtil::read(json["name"], data.name));
        CHECK_RESULT(JsonParseUtil::read(json["assetServer"], data.assetServer));
        if(json.isMember("graphics"))
            CHECK_RESULT(JsonParseUtil::read(json["graphics"], data.graphics));
        data.setSaved();
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const BraneProjectData& data)
    {
        CHECK_RESULT(JsonParseUtil::write(json["name"], data.name));
        CHECK_RESULT(JsonParseUtil::write(json["assetServer"], data.assetServer));
        CHECK_RESULT(JsonParseUtil::write(json["graphics"], data.graphics));
        return Ok<void>();
    }
};

Result<BraneProject> BraneProject::load(const std::filesystem::path& filepath, bool initProject)
{
    BraneProject proj(filepath.parent_path().make_preferred());
    Json::Value file;
    try
    {
        if(!FileManager::readFile(filepath.string(), file))
        {
            return Err("Could not open " + filepath.string());
        }
    }
    catch(const std::exception& e)
    {
        return Err("Error opening " + filepath.string() + ". " + e.what());
    }

    auto res = JsonSerializer<BraneProjectData>::read(file, &proj._data);
    if(!res)
        return Err(res.err().toString());
    if(initProject)
        proj.initLoaded();

    return Ok(std::move(proj));
}

Result<BraneProject> BraneProject::create(const std::string& projectName, const std::filesystem::path& directory)
{
    Json::Value file;
    auto defaultProjRes = BraneProject::load("defaultAssets/defaultProjectFile.brane", false);
    if(!defaultProjRes)
        return Err("Could not load default project! " + defaultProjRes.err());

    auto proj = defaultProjRes.ok();
    proj._data->name->set(projectName).now();
    proj._root = (directory / projectName).make_preferred();

    std::filesystem::create_directories(proj.root());
    std::filesystem::create_directory(proj.root() / "assets");
    std::filesystem::create_directory(proj.root() / "cache");
    proj.initLoaded();
    proj.save();
    return Ok(std::move(proj));
}

void BraneProject::save()
{
    auto openAssets = _openAssets.lock();
    auto openAsset = openAssets->begin();
    while(openAsset != openAssets->end())
    {
        if((*openAsset).second->unsavedChanges())
            (*openAsset).second->save();
        if((*openAsset).second.use_count() <= 1)
            openAsset = openAssets->erase(openAsset);
        else
            ++openAsset;
    }

    Json::Value jsonData;
    auto serializeRes = JsonSerializer<BraneProjectData>::write(jsonData, &_data);
    assert(serializeRes);

    FileManager::writeFile((_root / (*_data->name->value() + ".brane")).string(), jsonData);
}

std::filesystem::path BraneProject::root()
{
    return _root;
}

void BraneProject::initLoaded()
{
    _indexer->start(_root / "assets");
    save();
}

bool BraneProject::unsavedChanges() const
{
    if(_data->unsavedChanges())
        return true;
    auto openAssets = _openAssets.lockShared();
    for(auto& asset : *openAssets)
    {
        if(asset.second->unsavedChanges())
            return true;
    }
    return false;
}

BraneProjectData& BraneProject::data()
{
    return &_data;
}

Option<std::shared_ptr<EditorAsset>> BraneProject::getEditorAsset(const AssetID& id)
{
    if(id.empty())
        return None();
    auto path = _indexer->getAssetPath(id);
    if(!path)
        return None();
    return getEditorAsset(path.value());
}

Option<std::shared_ptr<EditorAsset>> BraneProject::getEditorAsset(const std::filesystem::path& path)
{
    auto loadRes = EditorAsset::loadAsset(path);
    if(!loadRes)
    {
        Runtime::error(std::format("Failed to load asset: {}", loadRes.err()));
        return None();
    }
    return Some<std::shared_ptr<EditorAsset>>(loadRes.ok());
}

AssetID BraneProject::getDefaultAsset(CreateAssetType type)
{
    AssetID existingAsset;
    switch(type)
    {
        case CreateAssetType::VertexShader:
            existingAsset = *_data->graphics->defaultVertexShader->value();
            break;
        case CreateAssetType::FragmentShader:
            existingAsset = *_data->graphics->defaultFragmentShader->value();
            break;
        case CreateAssetType::Material:
            existingAsset = *_data->graphics->defaultMaterial->value();
            break;
    }
    if(!existingAsset.empty())
        return existingAsset;
    std::filesystem::path defaultAssetDir = _root / "assets" / "defaults";
    std::string defaultName = "default";
    switch(type)
    {
        case CreateAssetType::VertexShader:
            defaultAssetDir /= "shaders";
            defaultName += "Vert";
            break;
        case CreateAssetType::FragmentShader:
            defaultAssetDir /= "shaders";
            defaultName += "Frag";
            break;
        case CreateAssetType::Material:
            defaultAssetDir /= "materials";
            defaultName += "Mat";
            break;
    }
    auto res = createAsset(defaultName, defaultAssetDir, type);
    if(!res)
    {
        Runtime::error(std::format("Failed to create default asset: {}", res.err()));
        return AssetID();
    }
    auto id = *res.ok()->metadata().begin()->second->exportId->value();

    switch(type)
    {
        case CreateAssetType::VertexShader:
            _data->graphics->defaultVertexShader->set(id).now();
            break;
        case CreateAssetType::FragmentShader:
            _data->graphics->defaultFragmentShader->set(id).now();
            break;
        case CreateAssetType::Material:
            _data->graphics->defaultMaterial->set(id).now();
            break;
    }
    save();
    // Return the first exported asset for now
    return id;
}

Result<std::shared_ptr<EditorAsset>>
BraneProject::createAsset(std::string_view name, std::filesystem::path path, CreateAssetType type)
{

    std::filesystem::path source = std::filesystem::current_path() / "defaultAssets";
    switch(type)
    {
        case CreateAssetType::VertexShader:
        case CreateAssetType::FragmentShader:
        {
            auto shaderSrc = source / "shaders";
            if(type == CreateAssetType::VertexShader)
            {
                shaderSrc /= "default.vert";
                path /= std::format("{}.vert", name);
            }
            else
            {
                shaderSrc /= "default.frag";
                path /= std::format("{}.frag", name);
            }
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path());
            std::filesystem::copy_file(shaderSrc, path, ec);
            if(ec)
                return Err("Could not copy default shader code: " + ec.message());
            auto asset = getEditorAsset(path);
            if(asset)
            {
                auto ret = asset.value();
                ret->save();
                return Ok(ret);
            }
            else
                return Err<std::string>("Could not load created shader");
        }
        break;
        case CreateAssetType::Material:
        {
            path = path / std::format("{}.material", name);
            auto material = std::make_shared<MaterialAssetSource>(path);
            auto vertShader = getDefaultAsset(CreateAssetType::VertexShader);
            Runtime::log("Found default vert shader: " + vertShader.toString());
            material->vertexShader->set(vertShader).now();
            material->fragmentShader->set(getDefaultAsset(CreateAssetType::FragmentShader)).now();
            auto am = Runtime::getModule<AssetManager>();
            /*// I'm going to be really lazy here and just block this thread until we get the shader asset, refactor
             * with*/
            /*// pipelines*/
            /*auto res = std::make_shared<Result<std::shared_ptr<ShaderAsset>>>(Err<std::string>("No value"));*/
            /*auto condition = std::make_shared<std::atomic_bool>(true);*/
            /*_indexer->indexAssets();*/
            /*am->fetchAsset<ShaderAsset>(vertShader)*/
            /*    .then([condition, res](std::shared_ptr<ShaderAsset> asset) {*/
            /*    condition->store(false);*/
            /*    *res = std::move(Ok(asset));*/
            /*}).onError([condition, res](std::string err) {*/
            /*    *res = std::move(Err(err));*/
            /*    condition->store(false);*/
            /*});*/
            /*while(condition->load())*/
            /*{*/
            /*    Runtime::log("Still waiting on vertexShader");*/
            /*    std::this_thread::sleep_for(std::chrono::milliseconds(250));*/
            /*}*/
            /*if(res->isErr())*/
            /*    return Err(std::format("Failed to load or build default vert shader: {}", res->err()));*/
            /*material->validateProperties(res->ok());*/
            material->setUnsaved();
            auto saveRes = material->save();
            if(!saveRes)
                return Err(std::format("Failed to save new asset: {}", saveRes.err()));
            auto asset = getEditorAsset(path);
            if(asset)
                return Ok(asset.value());
            else
                return Err<std::string>("Could not load created shader");
        }
        break;
    }
    return Err<std::string>("Type not handled");
}

AssetIndexer& BraneProject::indexer()
{
    return &_indexer;
}
