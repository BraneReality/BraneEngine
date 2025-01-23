//
// Created by eli on 8/14/2022.
//

#include "braneProject.h"
#include "assets/assetManager.h"
#include "assets/editorAsset.h"
#include "editor.h"
#include "fileManager/fileManager.h"
#include "runtime/runtime.h"

BraneAssetServerInfo::BraneAssetServerInfo() : address("localhost"), port(2001) {};

void BraneAssetServerInfo::initMembers(Option<std::shared_ptr<TrackedType>> parent)
{
    TrackedType::initMembers(parent);
    auto p = Some(shared_from_this());
    address->initMembers(p);
    port->initMembers(p);
}

BraneProjectData::BraneProjectData() : name("New project") {}

void BraneProjectData::initMembers(Option<std::shared_ptr<TrackedType>> parent)
{
    TrackedType::initMembers(parent);
    auto p = Some(shared_from_this());
    name->initMembers(p);
    assetServer->initMembers(p);
}

BraneProject::BraneProject(std::filesystem::path root) : _root(root), _indexer(), _data()
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
        CHECK_RESULT(JsonParseUtil::write(json["port"], data.address));
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
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const BraneProjectData& data)
    {
        CHECK_RESULT(JsonParseUtil::write(json["name"], data.name));
        CHECK_RESULT(JsonParseUtil::write(json["assetServer"], data.assetServer));
        return Ok<void>();
    }
};

Result<BraneProject> BraneProject::load(const std::filesystem::path& filepath, bool initProject)
{
    BraneProject proj(filepath.parent_path());
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
    proj._data->name->set(projectName).forward();
    proj._root = directory / projectName;

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
    _indexer.start(_root / "assets");
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
    auto path = _indexer.getAssetPath(id);
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

AssetIndexer& BraneProject::indexer()
{
    return _indexer;
}
