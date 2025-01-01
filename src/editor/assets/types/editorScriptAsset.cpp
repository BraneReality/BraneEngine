#include <mutex>
#include "assets/types/scriptAsset.h"
#include "editor/braneProject.h"
#include "editor/editor.h"
#include "editorScriptAsset.h"
#include "fileManager/fileManager.h"
#include "graphics/shader.h"
#include "runtime/runtime.h"
#include "utility/hex.h"

EditorScriptAsset::EditorScriptAsset(const std::filesystem::path& file, BraneProject& project)
    : EditorAsset(file, project)
{
    // Generate default
    if(!std::filesystem::exists(_file))
    {
        _json.data()["source"] = "";
    }
}

void EditorScriptAsset::updateSource(const std::filesystem::path& source)
{
    std::filesystem::path relPath = std::filesystem::relative(source, _file.parent_path()).string();
    _json.data()["source"] = relPath.string();
    std::string hash = FileManager::fileHash(source);
    bool changed = _json.data().get("lastSourceHash", "") != hash;

    if(changed)
    {
        Runtime::log("Extracting shader attributes for " + name());
        ShaderCompiler::ShaderAttributes attributes;
        std::string luaSource;
        std::filesystem::path dir = std::filesystem::path{source}.remove_filename();
        auto includer = std::make_unique<ShaderIncluder>();
        includer->addSearchDir(dir);
        if(FileManager::readFile(source, luaSource))
        {
            // TODO validate script and have it define components/pipelines
        }
        else
            Runtime::error("Failed to load script");
    }
    save();
}

Asset* EditorScriptAsset::buildAsset(const AssetID& id) const
{
    assert(id.toString() == _json["id"].asString());
    if(_json["source"].asString().empty())
    {
        Runtime::error("Shader source not set for " + _json["name"].asString());
        return nullptr;
    }
    std::filesystem::path source = _file.parent_path() / _json["source"].asString();
    std::string fileSuffix = source.extension().string();

    ScriptAsset* script = new ScriptAsset();
    script->id = id;
    script->name = name();

    std::string scriptCode;
    if(!FileManager::readFile(source, scriptCode))
    {
        Runtime::error("Failed to open shader source: " + source.string());
        return nullptr;
    }

    script->scriptText = std::move(scriptCode);


    return script;
}

std::vector<std::pair<AssetID, AssetType>> EditorScriptAsset::containedAssets() const
{
    std::vector<std::pair<AssetID, AssetType>> deps;
    auto res = AssetID::parse(_json["id"].asString());
    if(!res)
    {
        Runtime::error(std::format("Script Asset at {} has invalid id", _file.string()));
        return deps;
    }
    deps.emplace_back(res.ok(), AssetType::script);
    return std::move(deps);
}

void EditorScriptAsset::createDefaultSource()
{
    std::filesystem::path path = _file;
    path.remove_filename();
    path /= name() + ".lua";

    std::filesystem::path source = std::filesystem::current_path() / "defaultAssets" / "scripts" / "default.lua";
    if(!std::filesystem::exists(source))
    {
        Runtime::error("Could not find default script template at: " + source.string());
        return;
    }
    std::error_code ec;
    std::filesystem::copy_file(source, path, ec);
    if(ec)
    {
        Runtime::error("Could not copy default script code: " + ec.message());
        return;
    }
    updateSource(path);
}
