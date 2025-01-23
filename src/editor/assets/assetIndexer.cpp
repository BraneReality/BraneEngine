#include "assetIndexer.h"
#include "editor/assets/editorAsset.h"
#include "runtime/runtime.h"
#include <efsw/efsw.hpp>

FileListener::FileListener(AssetIndexer& indexer) : indexer(indexer) {};

void FileListener::handleFileAction(efsw::WatchID watchid,
                                    const std::string& dir,
                                    const std::string& filename,
                                    efsw::Action action,
                                    std::string oldFilename)
{
    indexer.indexAssets(); // Implement efficient things later, and debouncing. Debouncing is good.
    switch(action)
    {
        case efsw::Actions::Add:
            Runtime::log(std::format("DIR ({}) FILE ({}) had event Add", dir, filename));
            break;
        case efsw::Actions::Delete:
            Runtime::log(std::format("DIR ({}) FILE ({}) had event Delete", dir, filename));
            break;
        case efsw::Actions::Modified:
            Runtime::log(
                std::format("DIR ({}) FILE ({}) had event Modified (old name: {})", dir, filename, oldFilename));
            break;
        case efsw::Actions::Moved:
            Runtime::log(std::format("DIR ({}) FILE ({}) had event Moved (old name: {})", dir, filename, oldFilename));
            break;
        default:
            Runtime::error(std::format("DIR ({}) FILE ({}) had event that was not handled!", dir, filename));
    }
}

AssetIndexer::AssetIndexer() {}

void AssetIndexer::start(std::filesystem::path watchDir)
{
    _watchDir = watchDir;
    _fileWatcher = std::make_unique<efsw::FileWatcher>();
    _listener = std::make_unique<FileListener>(*this);
    _fileWatcher->addWatch(watchDir.string(), _listener.get(), true);
    indexAssets();
}

void AssetIndexer::indexPath(const std::filesystem::path& path)
{
    auto paths = _assetPaths.lock();
    auto loadRes = EditorAsset::loadAsset(path);
    if(loadRes)
    {
        // This is our bootleg way to validate assets have metadata and thus IDs
        auto asset = loadRes.ok();
        asset->save();

        for(auto& ca : asset->exportedAssets())
        {
            (*paths).insert({ca.first, {ca.first, path, ca.second}});
        }
    }
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

            for(auto& ca : asset->exportedAssets())
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
    return Some(path->second.sourcePath);
}

std::vector<AssetLocation> AssetIndexer::searchExporetedAssets(std::string_view name, Option<AssetType> type) const
{
    // Anyone who wants to refactor this to make it not the bare minumum and use something like fuzzy searching, be my
    // guest!
    std::vector<AssetLocation> result;
    auto assetPaths = _assetPaths.lockShared();
    for(auto& loc : *assetPaths)
    {
        if((name.empty() || loc.second.sourcePath.string().find(name) != std::string::npos) &&
           (type.isNone() || type.value() == loc.second.type))
            result.push_back(loc.second);
    }
    return result;
}
