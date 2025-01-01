//
// Created by eli on 8/18/2022.
//

#include "editorAsset.h"
#include <sstream>
#include "assets/asset.h"
#include "editor/assets/types/editorAssemblyAsset.h"
#include "editor/assets/types/editorChunkAsset.h"
#include "editor/assets/types/editorImageAsset.h"
#include "editor/assets/types/editorMaterialAsset.h"
#include "editor/assets/types/editorScriptAsset.h"
#include "editor/assets/types/editorShaderAsset.h"
#include "editor/braneProject.h"
#include "editor/editor.h"
#include "fileManager/fileManager.h"

EditorAsset::EditorAsset(const std::filesystem::path& file, BraneProject& project)
    : _file(file), _project(project), _json(project.editor().jsonTracker())
{
    auto ext = file.extension().string();
    if(ext.size() > 1)
        _type.set(ext.substr(1));
    _name = file.stem().string();
    load();
}

bool EditorAsset::unsavedChanges() const
{
    return _json.dirty();
}

bool EditorAsset::load()
{
    try
    {
        if(!FileManager::readFile(_file, _json.data()))
        {
            Runtime::log("Creating " + _file.string());
            _json.data()["id"] = _project.newAssetID(_file, _type).toString();
            return false;
        }
        if(_json.data().get("id", "null").asString() == "null")
        {
            _json.data()["id"] = _project.newAssetID(_file, _type).toString();
            _json.markDirty();
        }
    }
    catch(const std::exception& e)
    {
        Runtime::error("Could not parse " + _file.string() + "!\n" + e.what());
        _json.data()["failedLoad"] = true;
        return false;
    }
    return true;
}

void EditorAsset::save()
{
    if(_json.data().isMember("failedLoad"))
    {
        Runtime::warn("Tried to save file that did not load correctly! \nAborting to prevent overwrite with further "
                      "invalid data");
        return;
    }
    FileManager::writeFile(_file, _json.data());
    _json.markClean();
    auto* builtAsset = buildAsset(AssetID::parse(_json["id"].asString()).ok());
    if(builtAsset)
        _project.editor().cache().cacheAsset(builtAsset);
    else
        Runtime::warn("Could not build and cache " + name());
    Runtime::log("Saved " + _file.string());
}

std::string EditorAsset::hash(const AssetID& id)
{
    if(unsavedChanges())
        save();
    if(!_project.editor().cache().hasAsset(id))
        _project.editor().cache().cacheAsset(buildAsset(id));

    return _project.editor().cache().getAssetHash(id);
}

VersionedJson& EditorAsset::json()
{
    return _json;
}

EditorAsset* EditorAsset::openUnknownAsset(const std::filesystem::path& path, BraneProject& project)
{
    auto ext = path.extension();
    if(ext == ".shader")
        return new EditorShaderAsset(path, project);
    if(ext == ".material")
        return new EditorMaterialAsset(path, project);
    if(ext == ".assembly")
        return new EditorAssemblyAsset(path, project);
    if(ext == ".chunk")
        return new EditorChunkAsset(path, project);
    if(ext == ".image")
        return new EditorImageAsset(path, project);
    if(ext == ".script")
        return new EditorScriptAsset(path, project);
    Runtime::error("Extension " + ext.string() + " not recognised");
    return nullptr;
}

const AssetType& EditorAsset::type() const
{
    return _type;
}

const std::string& EditorAsset::name() const
{
    return _name;
}

const std::filesystem::path& EditorAsset::file() const
{
    return _file;
}

AssetID EditorAsset::id() const
{
    return AssetID::parse(_json["id"].asString()).ok();
}
