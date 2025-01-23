//
// Created by eli on 8/24/2022.
//

#include "assetCache.h"
#include "editor/braneProject.h"
#include "fileManager/fileManager.h"
#include "utility/hex.h"

void AssetCache::setProject(BraneProject* project)
{
    _project = project;
}

std::filesystem::path AssetCache::getPath(const AssetID& id)
{
    assert(_project);
    return _project->root() / "cache" / (id.toString() + ".bin");
}

void AssetCache::cacheAsset(const Asset* asset)
{
    if(!asset)
    {
        Runtime::warn("Tried to cache nonexistent asset");
        return;
    }
    FileManager::writeAsset(asset, getPath(asset->id));
}

void AssetCache::deleteCachedAsset(const AssetID& asset)
{
    std::filesystem::path path = getPath(asset);
    if(!std::filesystem::exists(path))
        return;

    FileManager::deleteFile(path);
}

Asset* AssetCache::getAsset(const AssetID& asset)
{
    std::filesystem::path path = getPath(asset);
    if(!std::filesystem::exists(path))
        return nullptr;

    return FileManager::readUnknownAsset(path);
}

bool AssetCache::hasAsset(const AssetID& asset)
{
    return std::filesystem::exists(getPath(asset));
}

std::string AssetCache::getAssetHash(const AssetID& asset)
{
    return FileManager::checksum(getPath(asset));
}
