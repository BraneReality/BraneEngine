//
// Created by eli on 8/24/2022.
//

#include "assetCache.h"
#include "fileManager/fileManager.h"
#include "editor/braneProject.h"
#include "utility/hex.h"

void AssetCache::setProject(BraneProject* project)
{
	_project = project;
}

std::filesystem::path AssetCache::getPath(const AssetID& id)
{
	assert(_project);
	return _project->projectDirectory() / "cache" / ( toHex(id.id) + ".bin");
}

void AssetCache::cacheAsset(const Asset* asset)
{
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

