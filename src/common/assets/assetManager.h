#pragma once

#include <runtime/module.h>

#include "asset.h"
#include <unordered_set>
#include <utility/asyncData.h>
#include <utility/asyncQueue.h>

class EntityManager;

class AssetManager : public Module
{
  public:
    using FetchCallback = std::function<AsyncData<Asset*>(const AssetID& id, bool incremental)>;

    enum class LoadState : uint8_t
    {
        unloaded = 0,
        failed = 1,
        requested = 2,
        awaitingDependencies = 3,
        usable = 4,
        loaded = 5
    };

    struct AssetData
    {
        std::unique_ptr<Asset> asset;
        uint32_t useCount = 0;
        uint32_t unloadedDependencies = 0;
        LoadState loadState = LoadState::unloaded;
        std::unordered_set<HashedAssetID> usedBy;
    };

  private:
    std::mutex _assetLock;
    std::unordered_map<HashedAssetID, std::unique_ptr<AssetData>> _assets;
    std::unordered_map<HashedAssetID, std::vector<std::function<void(Asset*)>>> _awaitingLoad;

    size_t _nativeComponentID = 0;

    template<typename T>
    void addNativeComponent(EntityManager& em);

    // To account for different ways of fetching assets for different build targets, this function is defined multiple
    // times
    AsyncData<Asset*> fetchAssetInternal(const AssetID& id, bool incremental);

  public:
    AssetManager();

    template<typename T>
    T* getAsset(const AssetID& id)
    {
        static_assert(std::is_base_of<Asset, T>());
        std::scoped_lock lock(_assetLock);
        if(_assets.count(id))
            return (T*)(_assets[id]->asset.get());
        return nullptr;
    }

    AsyncData<Asset*> fetchAsset(const AssetID& id, bool incremental = false);

    template<typename T>
    AsyncData<T*> fetchAsset(const AssetID& id)
    {
        static_assert(std::is_base_of<Asset, T>());
        AsyncData<T*> asset;
        fetchAsset(id, std::is_base_of<IncrementalAsset, T>()).then([asset](Asset* a) {
            asset.setData(std::move((T*)a));
        });
        return asset;
    }

    void reloadAsset(Asset* asset);

    bool hasAsset(const AssetID& id);

    void fetchDependencies(Asset* asset, std::function<void(bool success)> callback);

    bool dependenciesLoaded(const Asset* asset) const;

    std::vector<const Asset*> nativeAssets(AssetType type);

    static const char* name();

    void start() override;
};
