//
// Created by eli on 8/14/2022.
//

#include "braneProject.h"
#include <fstream>
#include "assets/assetManager.h"
#include "assets/editorAsset.h"
#include "assets/types/editorAssemblyAsset.h"
#include "assets/types/editorImageAsset.h"
#include "assets/types/editorShaderAsset.h"
#include "editor.h"
#include "fileManager/fileManager.h"
#include "fileManager/fileWatcher.h"
#include "runtime/runtime.h"
#include "utility/hex.h"

BraneProject::BraneProject(Editor& editor) : _editor(editor), _file(editor.jsonTracker()) {}

BraneProject::~BraneProject() {}

void BraneProject::loadDefault()
{
    Json::Value file;
    try
    {
        if(!FileManager::readFile("defaultAssets/defaultProjectFile.brane", file))
        {
            Runtime::error("Could not open default project file");
            throw std::runtime_error("Could not open default project file");
        }
    }
    catch(const std::exception& e)
    {
        Runtime::error("Error parsing default project file: " + (std::string)e.what());
        throw std::runtime_error("Error  parsing default project file!");
    }
    _file.initialize(file);
}

bool BraneProject::load(const std::filesystem::path& filepath)
{
    _filepath = filepath;
    Json::Value file;
    try
    {
        if(!FileManager::readFile(filepath.string(), file))
        {
            Runtime::error("Could not open " + filepath.string());
            return false;
        }
    }
    catch(const std::exception& e)
    {
        Runtime::error("Error opening " + filepath.string() + ". " + e.what());
        return false;
    }
    _file.initialize(file);
    initLoaded();
    return true;
}

void BraneProject::create(const std::string& projectName, const std::filesystem::path& directory)
{
    loadDefault();
    _file.data()["info"]["name"] = projectName;
    _filepath = directory;
    _filepath = _filepath / projectName / (projectName + ".brane");

    std::filesystem::create_directories(projectDirectory());
    std::filesystem::create_directory(projectDirectory() / "assets");
    std::filesystem::create_directory(projectDirectory() / "cache");
    initLoaded();
    save();
}

void BraneProject::save()
{
    if(!_loaded)
        return;
    auto openAsset = _openAssets.begin();
    while(openAsset != _openAssets.end())
    {
        if((*openAsset).second->unsavedChanges())
            (*openAsset).second->save();
        if((*openAsset).second.use_count() <= 1)
            openAsset = _openAssets.erase(openAsset);
        else
            ++openAsset;
    }

    Json::Value& assets = _file.data()["assets"];
    // Clean up broken filepaths
    for(const std::string& id : assets.getMemberNames())
    {
        std::filesystem::path path = projectDirectory() / "assets" / assets[id]["path"].asString();
        if(!std::filesystem::exists(path))
            assets.removeMember(id);
    }

    FileManager::writeFile(_filepath.string(), _file.data());
    _file.markClean();
}

bool BraneProject::loaded() const { return _loaded; }

std::filesystem::path BraneProject::projectDirectory() { return _filepath.parent_path(); }

void BraneProject::initLoaded()
{
    refreshAssets();

    Json::Value& assets = _file.data()["assets"];
    if(!_file.data().isMember("assetIdCounter"))
        _file.data()["assetIdCounter"] = 0;

    _fileWatcher = std::make_unique<FileWatcher>();
    _fileWatcher->loadCache(projectDirectory() / "cache" / "changeCache");
    _fileWatcher->watchDirectory(projectDirectory() / "assets");
    _fileWatcher->addFileWatcher(".gltf", [this](const std::filesystem::path& path) {
        Runtime::log("loading gltf: " + path.string());
        std::filesystem::path assetPath = path;
        assetPath.replace_extension(".assembly");

        bool isOpen = false;
        EditorAssemblyAsset* assembly;
        if(_openAssets.count(assetPath.string()))
        {
            assembly = (EditorAssemblyAsset*)_openAssets.at(assetPath.string()).get();
            isOpen = true;
        }
        else
            assembly = new EditorAssemblyAsset(assetPath, *this);

        assembly->linkToGLTF(path);

        registerAssetLocation(assembly);
        _editor.cache().deleteCachedAsset(AssetID{assembly->json()["id"].asString()});

        if(!isOpen)
            delete assembly;
    });
    _fileWatcher->addFileWatcher(".glb", [this](const std::filesystem::path& path) {
        Runtime::log("loading gltf: " + path.string());
        std::filesystem::path assetPath = path;
        assetPath.replace_extension(".assembly");

        bool isOpen = false;
        EditorAssemblyAsset* assembly;
        if(_openAssets.count(assetPath.string()))
        {
            assembly = (EditorAssemblyAsset*)_openAssets.at(assetPath.string()).get();
            isOpen = true;
        }
        else
            assembly = new EditorAssemblyAsset(assetPath, *this);

        assembly->linkToGLTF(path);

        registerAssetLocation(assembly);
        _editor.cache().deleteCachedAsset(AssetID{assembly->json()["id"].asString()});

        if(!isOpen)
            delete assembly;
    });
    _fileWatcher->addFileWatcher(".vert", [this](const std::filesystem::path& path) {
        Runtime::log("loading fragment shader: " + path.string());
        std::filesystem::path assetPath = path;
        assetPath.replace_extension(".shader");

        bool isOpen = _openAssets.count(assetPath.string());
        if(!isOpen)
            _openAssets.insert({assetPath.string(), std::make_shared<EditorShaderAsset>(assetPath, *this)});
        std::shared_ptr<EditorShaderAsset> shaderAsset =
            std::dynamic_pointer_cast<EditorShaderAsset>(_openAssets.at(assetPath.string()));

        shaderAsset->updateSource(path);

        registerAssetLocation(shaderAsset.get());
        _editor.reloadAsset(shaderAsset);

        if(!isOpen)
            _openAssets.erase(assetPath.string());
    });
    _fileWatcher->addFileWatcher(".frag", [this](const std::filesystem::path& path) {
        Runtime::log("loading vertex shader: " + path.string());
        std::filesystem::path assetPath = path;
        assetPath.replace_extension(".shader");

        bool isOpen = _openAssets.count(assetPath.string());
        if(!isOpen)
            _openAssets.insert({assetPath.string(), std::make_shared<EditorShaderAsset>(assetPath, *this)});
        std::shared_ptr<EditorShaderAsset> shaderAsset =
            std::dynamic_pointer_cast<EditorShaderAsset>(_openAssets.at(assetPath.string()));

        shaderAsset->updateSource(path);

        registerAssetLocation(shaderAsset.get());
        _editor.reloadAsset(shaderAsset);

        if(!isOpen)
            _openAssets.erase(assetPath.string());
    });
    _fileWatcher->addFileWatcher(".png", [this](const std::filesystem::path& path) {
        Runtime::log("loading image: " + path.string());
        std::filesystem::path assetPath = path;
        assetPath.replace_extension(".image");

        bool isOpen = _openAssets.count(assetPath.string());
        if(!isOpen)
            _openAssets.insert({assetPath.string(), std::make_shared<EditorImageAsset>(assetPath, *this)});
        std::shared_ptr<EditorImageAsset> imageAsset =
            std::dynamic_pointer_cast<EditorImageAsset>(_openAssets.at(assetPath.string()));

        imageAsset->updateSource(path);

        registerAssetLocation(imageAsset.get());
        _editor.reloadAsset(imageAsset);

        if(!isOpen)
            _openAssets.erase(assetPath.string());
    });
    _fileWatcher->scanForChanges(true);
    _loaded = true;
    save();
}

bool BraneProject::unsavedChanges() const
{
    if(_file.dirty())
        return true;
    for(auto& asset : _openAssets)
    {
        if(asset.second->unsavedChanges())
            return true;
    }
    return false;
}

VersionedJson& BraneProject::json() { return _file; }

std::shared_ptr<EditorAsset> BraneProject::getEditorAsset(const AssetID& id)
{
    if(id.null())
        return nullptr;
    if(!_file["assets"].isMember(id.string()))
        return nullptr;
    std::filesystem::path path = projectDirectory() / "assets" / _file["assets"][id.string()]["path"].asString();
    return getEditorAsset(path);
}

std::shared_ptr<EditorAsset> BraneProject::getEditorAsset(const std::filesystem::path& path)
{
    if(_openAssets.count(path.string()))
        return _openAssets.at(path.string());
    auto asset = std::shared_ptr<EditorAsset>(EditorAsset::openUnknownAsset(path, *this));
    if(asset)
        _openAssets.insert({path.string(), asset});
    return asset;
}

Editor& BraneProject::editor() { return _editor; }

AssetID BraneProject::newAssetID(const std::filesystem::path& editorAsset, AssetType type)
{
    Json::Value& assets = _file.data()["assets"];
    auto id = _file["assetIdCounter"].asUInt();
    std::string testID = "/" + toHex(id);
    while(assets.isMember(testID))
        testID = "/" + toHex(++id);
    _file.data()["assetIdCounter"] = id;

    assets[testID]["path"] = std::filesystem::relative(editorAsset, projectDirectory() / "assets").string();
    assets[testID]["type"] = type.toString();

    return AssetID(testID);
}

FileWatcher* BraneProject::fileWatcher() { return _fileWatcher.get(); }

void BraneProject::registerAssetLocation(const EditorAsset* asset)
{
    assert(asset);
    assert(std::filesystem::exists(asset->file()));
    Json::Value& assets = _file.data()["assets"];
    for(std::pair<AssetID, AssetType>& a : asset->containedAssets())
    {
        assert(!a.first.null());
        assets[a.first.string()]["path"] =
            std::filesystem::relative(asset->file(), projectDirectory() / "assets").string();
        assets[a.first.string()]["type"] = a.second.toString();
    }
}

void BraneProject::refreshAssets()
{
    Json::Value& assets = _file.data()["assets"];
    // Delete paths that no longer exist
    for(const std::string& id : assets.getMemberNames())
    {
        std::filesystem::path path = projectDirectory() / "assets" / assets[id]["path"].asString();
        if(!std::filesystem::exists(path))
            assets.removeMember(id);
    }

    std::unordered_set<std::string> assetTypes = {".shader", ".material", ".assembly", ".image"};
    for(auto& file : std::filesystem::recursive_directory_iterator{projectDirectory() / "assets"})
    {
        if(!file.is_regular_file())
            continue;
        if(!assetTypes.count(file.path().extension().string()))
            continue;
        EditorAsset* asset = nullptr;
        try
        {
            asset = EditorAsset::openUnknownAsset(file, *this);
        }
        catch(const std::exception& e)
        {
            Runtime::error("Could not open asset " + file.path().string() + " error: " + e.what());
            continue;
        }
        if(!asset)
        {
            Runtime::error("Could not automatically open asset with extension " + file.path().extension().string());
            continue;
        }
        registerAssetLocation(asset);
        delete asset;
    }
}

std::vector<std::pair<AssetID, std::filesystem::path>> BraneProject::searchAssets(const std::string& query,
                                                                                  AssetType type)
{
    std::vector<std::pair<AssetID, std::filesystem::path>> assets;
    auto nativeAssets = Runtime::getModule<AssetManager>()->nativeAssets(type);
    for(auto& asset : nativeAssets)
        assets.emplace_back(asset->id, asset->name);
    try
    {
        for(auto& assetID : _file["assets"].getMemberNames())
        {
            const Json::Value& asset = _file["assets"][assetID];
            std::filesystem::path path{asset["path"].asString()};
            if(type != AssetType::none && asset["type"] != type.toString())
                continue;
            if(!query.empty() && path.filename().string().find(query) == std::string::npos)
                continue;
            assets.emplace_back(assetID, path);
        }
    }
    catch(const std::exception& e)
    {
        Runtime::warn("Error searching assets, may be a problem with the project file: " + (std::string)e.what());
    }

    std::sort(assets.begin(), assets.end(), [](auto& a, auto& b) { return a.second.stem() < b.second.stem(); });
    return assets;
}

std::string BraneProject::getAssetName(const AssetID& id)
{
    if(!_file["assets"].isMember(id.string()))
        return "null";
    return std::filesystem::path{_file["assets"][id.string()]["path"].asString()}.stem().string();
}

std::vector<std::pair<AssetID, std::string>> BraneProject::getAssetHashes()
{
    std::vector<std::pair<AssetID, std::string>> hashes;
    auto ids = _file["assets"].getMemberNames();
    for(auto& idStr : ids)
    {
        AssetID id(idStr);
        hashes.emplace_back(std::move(id), getEditorAsset(id)->hash(id));
    }
    return hashes;
}
