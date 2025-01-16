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

EditorAsset::EditorAsset(const std::filesystem::path& file) : TrackedObject(None()), _file(file)
{
    auto ext = file.extension().string();
    if(ext.size() > 1)
        _type.set(ext.substr(1));
    _name = file.stem().string();
}

bool EditorAsset::unsavedChanges() const
{
    return changeDelta != 0 || changeVersion != saveVersion;
}

void EditorAsset::save()
{
    FileManager::writeFile(_file, _data.data());
    _data.markClean();
    auto* builtAsset = buildAsset(AssetID::parse(_data["id"].asString()).ok());
    if(builtAsset)
        _project.editor().cache().cacheAsset(builtAsset);
    else
        Runtime::warn("Could not build and cache " + name());
    Runtime::log("Saved " + _file.string());
}

std::string EditorAsset::hash() const
{
    if(unsavedChanges())
        save();
    if(!_project.editor().cache().hasAsset(id))
        _project.editor().cache().cacheAsset(buildAsset(id));

    return _project.editor().cache().getAssetHash(id);
}

VersionedJson& EditorAsset::data()
{
    return _data;
}

VersionedJson& EditorAsset::metadata()
{
    return _metadata;
}

EditorAsset* EditorAsset::openUnknownAsset(const std::filesystem::path& path)
{
    auto ext = path.extension();
    if(ext == ".shader")
        return new EditorShaderAsset(path);
    if(ext == ".material")
        return new EditorMaterialAsset(path);
    if(ext == ".assembly")
        return new EditorAssemblyAsset(path);
    if(ext == ".chunk")
        return new EditorChunkAsset(path);
    if(ext == ".image")
        return new EditorImageAsset(path);
    if(ext == ".script")
        return new EditorScriptAsset(path);
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

const std::filesystem::path& EditorAsset::dataPath() const
{
    return _file;
}

AssetID EditorAsset::id() const
{
    return AssetID::parse(_data["id"].asString()).ok();
}
