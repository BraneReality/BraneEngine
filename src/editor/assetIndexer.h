#pragma once
#include <filesystem>
#include "assets/assetID.h"
#include "assets/assetType.h"
#include "utility/mutex.h"
#include <efsw/efsw.hpp>
#include <unordered_map>

struct FileListener;

class AssetIndexer
{

    std::unique_ptr<efsw::FileWatcher> _fileWatcher;
    std::unique_ptr<FileListener> _listener;
    std::filesystem::path _watchDir;

    struct AssetLocation
    {
        AssetID id;
        std::filesystem::path sourcePath;
        AssetType type;
    };

    RwMutex<std::unordered_map<AssetID, AssetLocation>> _assetPaths;

  public:
    AssetIndexer(std::filesystem::path watchDir);

    void indexAssets();
    Option<std::filesystem::path> getAssetPath(const AssetID& id);
};
