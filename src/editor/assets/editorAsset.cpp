//
// Created by eli on 8/18/2022.
//

#include "editorAsset.h"
#include "assets/asset.h"
#include "editor/braneProject.h"
#include "editor/editor.h"
#include "fileManager/fileManager.h"

EditorAsset::EditorAsset(const std::filesystem::path& file, std::shared_ptr<AssetSource> source)
    : _source(std::move(source))
{
    auto ext = file.extension().string();
}

void EditorAsset::initMembers()
{
    _source->initMembers(Some(shared_from_this()));
    for(auto& md : _metadata)
        md.second->initMembers(Some(shared_from_this()));
}

Shared<AssetSource> EditorAsset::source() const
{
    return _source;
}

const std::unordered_map<AssetSourceID, Shared<AssetMetadata>>& EditorAsset::metadata() const
{
    return _metadata;
}

Option<Shared<AssetMetadata>> EditorAsset::getMetadata(AssetID id) const
{
    auto idEntry = _exportedToSource.find(id);
    if(idEntry != _exportedToSource.end())
        return getMetadata(idEntry->second);
    return None();
}

Option<Shared<AssetMetadata>> EditorAsset::getMetadata(AssetSourceID id) const
{
    auto entry = _metadata.find(id);
    if(entry != _metadata.end())
        return Some(entry->second);
    return None();
}

bool EditorAsset::unsavedChanges() const
{
    bool somethingChanged = _source->unsavedChanges();
    for(auto& m : _metadata)
        somethingChanged |= m.second->unsavedChanges();
    return somethingChanged;
}

void EditorAsset::save()
{
    if(!unsavedChanges())
        return;
    _source->save();
    _source->setSaved();

    Json::Value md;
    Json::Value& exports = md["exports"];
    for(auto& m : _metadata)
    {
        auto mdRes = m.second->serialize();
        if(!mdRes)
        {
            Runtime::error(std::format("Failed to serialize metadata: {}", mdRes.err()));
        }

        auto json = mdRes.ok();
        Runtime::log(std::format("Serialized metadata: {}", json.toStyledString()));
        exports.append(json);
        m.second->setSaved();
    }

    FileManager::writeFile(metadataPath(), md);
    Runtime::log(std::format("Wrote {}", metadataPath().string()));
}

std::string EditorAsset::hash() const
{
    return FileManager::checksum(_source->path);
}

Result<std::shared_ptr<EditorAsset>> EditorAsset::loadAsset(const std::filesystem::path& path)
{
    std::shared_ptr<EditorAsset> asset;
    auto ext = path.extension();


    if(ext == ".png" || ext == ".jpg" || ext == ".jpeg")
        asset =
            std::make_shared<EditorAsset>(path, (std::shared_ptr<AssetSource>)std::make_shared<ImageAssetSource>(path));

    if(!asset)
        return Err(std::format("Extension {} not recognised", ext.string()));

    if(std::filesystem::exists(asset->metadataPath()))
    {
        Json::Value metadata;
        if(FileManager::readFile(asset->metadataPath(), metadata))
        {
            if(metadata.isMember("exports") && metadata["exports"].isArray())
            {
                for(auto& e : metadata["exports"])
                {
                    Runtime::log(std::format("Deserializing: {}", e.toStyledString()));
                    auto parseRes = AssetMetadata::deserialize(e);
                    if(!parseRes)
                    {
                        Runtime::warn(std::format("Unable to parse metadata for {} reason: {}",
                                                  asset->metadataPath().string(),
                                                  parseRes.err()));
                        continue;
                    }
                    auto md = parseRes.ok();
                    asset->_metadata.insert({md->sourceId, Shared(md)});
                    asset->_exportedToSource.insert({*md->exportId->value(), md->sourceId});
                }
            }
        }
    }

    for(auto defined : asset->_source->exportedAssets())
    {
        if(asset->_metadata.contains(defined.first))
            continue;
        auto nmdRes = asset->_source->buildMetadata(defined.first);
        if(!nmdRes)
        {
            Runtime::error(std::format(
                "Unable to create default metadata for {} reason: ", asset->metadataPath().string(), nmdRes.err()));
            continue;
        }
        auto nmd = nmdRes.ok();
        asset->_metadata.insert({nmd->sourceId, Shared<AssetMetadata>(nmd)});
        asset->_exportedToSource.insert({*nmd->exportId->value(), nmd->sourceId});
    }

    asset->initMembers();
    return Ok(std::move(asset));
}

std::vector<std::pair<AssetID, AssetType>> EditorAsset::exportedAssets() const
{
    std::vector<std::pair<AssetID, AssetType>> ret;
    ret.reserve(_exportedToSource.size());
    for(auto& idPair : _metadata)
        ret.push_back({*idPair.second->exportId->value(), idPair.second->assetType()});
    return ret;
}

Result<std::shared_ptr<Asset>> EditorAsset::buildAsset(const AssetID& id) const
{
    auto sourceId = _exportedToSource.find(id);
    if(sourceId == _exportedToSource.end())
    {
        return Err(std::format(
            "No mapping from '{}' to a source id exported from {} was found", id.toString(), _source->path.string()));
    }
    return buildAsset(sourceId->second);
}

Result<std::shared_ptr<Asset>> EditorAsset::buildAsset(const AssetSourceID& id) const
{
    auto metadata = _metadata.find(id);
    if(metadata == _metadata.end())
    {
        return Err(std::format("No asset '{}' exported from '{}' or no metadata was found",
                               id.data.toStyledString(),
                               _source->path.string()));
    }
    return _source->buildAsset(metadata->second);
}

const std::filesystem::path& EditorAsset::sourcePath() const
{
    return _source->path;
}

std::filesystem::path EditorAsset::metadataPath() const
{
    return std::filesystem::path(_source->path).replace_extension(_source->path.extension().string() + ".b-meta");
}

std::string EditorAsset::name() const
{
    return _source->path.filename().string();
}
