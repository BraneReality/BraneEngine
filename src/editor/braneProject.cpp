//
// Created by eli on 8/14/2022.
//

#include "braneProject.h"
#include <fstream>
#include "assets/assetManager.h"
#include "assets/editorAsset.h"
#include "assets/types/editorAssemblyAsset.h"
#include "assets/types/editorImageAsset.h"
#include "assets/types/editorScriptAsset.h"
#include "assets/types/editorShaderAsset.h"
#include "editor.h"
#include "fileManager/fileManager.h"
#include "fileManager/fileWatcher.h"
#include "runtime/runtime.h"
#include "utility/hex.h"

BraneProject::BraneProject(Editor& editor) : _editor(editor), _data(editor.jsonTracker()) {}

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
    _data.initialize(file);
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
    _data.initialize(file);
    initLoaded();
    return true;
}

void BraneProject::create(const std::string& projectName, const std::filesystem::path& directory)
{
    loadDefault();
    _data.data()["info"]["name"] = projectName;
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

    FileManager::writeFile(_filepath.string(), _data.data());
    _data.markClean();
}

bool BraneProject::loaded() const
{
    return _loaded;
}

std::filesystem::path BraneProject::projectDirectory()
{
    return _filepath.parent_path();
}

void BraneProject::initLoaded()
{
    refreshAssets();

    Json::Value& assets = _data.data()["assets"];
    if(!_data.data().isMember("assetIdCounter"))
        _data.data()["assetIdCounter"] = 0;

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
        _editor.cache().deleteCachedAsset(AssetID::parse(assembly->data()["id"].asString()).ok());

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
        _editor.cache().deleteCachedAsset(AssetID::parse(assembly->data()["id"].asString()).ok());

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
    _fileWatcher->addFileWatcher(".lua", [this](const std::filesystem::path& path) {
        Runtime::log("loading script: " + path.string());
        std::filesystem::path assetPath = path;
        assetPath.replace_extension(".script");

        bool isOpen = _openAssets.count(assetPath.string());
        if(!isOpen)
            _openAssets.insert({assetPath.string(), std::make_shared<EditorScriptAsset>(assetPath, *this)});
        std::shared_ptr<EditorScriptAsset> scriptAsset =
            std::dynamic_pointer_cast<EditorScriptAsset>(_openAssets.at(assetPath.string()));

        scriptAsset->updateSource(path);

        registerAssetLocation(scriptAsset.get());
        _editor.reloadAsset(scriptAsset);

        if(!isOpen)
            _openAssets.erase(assetPath.string());
    });
    _fileWatcher->scanForChanges(true);
    _loaded = true;
    save();
}

bool BraneProject::unsavedChanges() const
{
    if(_data.dirty())
        return true;
    for(auto& asset : _openAssets)
    {
        if(asset.second->unsavedChanges())
            return true;
    }
    return false;
}

VersionedJson& BraneProject::json()
{
    return _data;
}

std::shared_ptr<EditorAsset> BraneProject::getEditorAsset(const AssetID& id)
{
    if(id.empty())
        return nullptr;
    if(!_data["assets"].isMember(id.toString()))
        return nullptr;
    std::filesystem::path path = projectDirectory() / "assets" / _data["assets"][id.toString()]["path"].asString();
    return getEditorAsset(path);
}

std::shared_ptr<EditorAsset> BraneProject::getEditorAsset(const std::filesystem::path& path)
{
    if(_openAssets.count(path.string()))
        return _openAssets.at(path.string());
    auto asset = std::shared_ptr<EditorAsset>(EditorAsset::loadAsset(path, *this));
    if(asset)
        _openAssets.insert({path.string(), asset});
    return asset;
}

Editor& BraneProject::editor()
{
    return _editor;
}

AssetID BraneProject::newAssetID(const std::filesystem::path& editorAsset, AssetType type)
{
    Json::Value& assets = _data.data()["assets"];
    auto id = _data["assetIdCounter"].asUInt();
    std::string testID = "/" + toHex(id);
    while(assets.isMember(testID))
        testID = "/" + toHex(++id);
    _data.data()["assetIdCounter"] = id;

    assets[testID]["path"] = std::filesystem::relative(editorAsset, projectDirectory() / "assets").string();
    assets[testID]["type"] = type.toString();

    return AssetID::parse(testID).ok();
}

FileWatcher* BraneProject::fileWatcher()
{
    return _fileWatcher.get();
}
