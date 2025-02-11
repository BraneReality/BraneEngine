#pragma once
#include <filesystem>
#include <memory>
#include "assets/assetID.h"
#include "assets/assetType.h"
#include "utility/mutex.h"
#include <efsw/efsw.hpp>
#include <unordered_map>

class AssetIndexer;

struct FileListener : efsw::FileWatchListener
{
    std::weak_ptr<AssetIndexer> indexer;
    FileListener(std::shared_ptr<AssetIndexer> indexer);

    void handleFileAction(efsw::WatchID watchid,
                          const std::string& dir,
                          const std::string& filename,
                          efsw::Action action,
                          std::string oldFilename) override;
};

struct AssetLocation
{
    AssetID id;
    std::filesystem::path sourcePath;
    AssetType type;
};

class AssetIndexer : public std::enable_shared_from_this<AssetIndexer>
{

    std::unique_ptr<efsw::FileWatcher> _fileWatcher;
    std::unique_ptr<FileListener> _listener;
    std::filesystem::path _watchDir;


    RwMutex<std::unordered_map<AssetID, AssetLocation>> _assetPaths;
    friend struct FileListener;

    void indexPath(const std::filesystem::path& path);

  public:
    void start(std::filesystem::path watchDir);

    void indexAssets();
    Option<std::filesystem::path> getAssetPath(const AssetID& id);
    std::vector<AssetLocation> searchExporetedAssets(std::string_view name, Option<AssetType> type) const;
};
