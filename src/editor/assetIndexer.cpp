#include "assetIndexer.h"
#include "editor/assets/editorAsset.h"
#include "runtime/runtime.h"
#include <efsw/efsw.hpp>

struct FileListener : efsw::FileWatchListener
{
    AssetIndexer& indexer;
    FileListener(AssetIndexer& indexer) : indexer(indexer) {};

    void handleFileAction(efsw::WatchID watchid,
                          const std::string& dir,
                          const std::string& filename,
                          efsw::Action action,
                          std::string oldFilename) override
    {
        switch(action)
        {
            case efsw::Actions::Add:
                Runtime::log(std::format("DIR ({}) FILE ({}) had event Add", dir, filename));
                break;
            case efsw::Actions::Delete:
                Runtime::log(std::format("DIR ({}) FILE ({}) had event Delete", dir, filename));
                break;
            case efsw::Actions::Modified:
                Runtime::log(std::format("DIR ({}) FILE ({}) had event Modified", dir, filename));
                break;
            case efsw::Actions::Moved:
                Runtime::log(std::format("DIR ({}) FILE ({}) had event Moved", dir, filename));
                break;
            default:
                Runtime::error(std::format("DIR ({}) FILE ({}) had event that was not handled!", dir, filename));
        }
    }
};

AssetIndexer::AssetIndexer(std::filesystem::path watchDir)
{
    _watchDir = watchDir;
    _fileWatcher = std::make_unique<efsw::FileWatcher>();
    _listener = std::make_unique<FileListener>(*this);
    _fileWatcher->addWatch(watchDir.string(), _listener.get(), true);
}

void AssetIndexer::indexAssets()
{
    auto paths = _assetPaths.lock();
    (*paths).clear();

    namespace fs = std::filesystem;
    for(const auto& entry : fs::recursive_directory_iterator(_watchDir))
    {
        if(!fs::is_regular_file(entry))
            continue;
        auto loadRes = EditorAsset::loadAsset(entry);
        if(loadRes)
        {
            // This is our bootleg way to validate assets have metadata and thus IDs
            auto asset = loadRes.ok();
            asset->save();

            for(auto& ca : asset->containedAssets())
            {
                (*paths).insert({ca.first, {ca.first, entry, ca.second}});
            }
        }
    }
}

Option<std::filesystem::path> AssetIndexer::getAssetPath(const AssetID& id)
{
    auto paths = _assetPaths.lockShared();
    auto path = (*paths).find(id);
    if(path == (*paths).end())
        return None();
    return Some(path->second);
}
