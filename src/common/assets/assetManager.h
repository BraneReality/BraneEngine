#pragma once

#include "utility/shared.h"
#include <runtime/module.h>

#include <mutex>
#include <variant>
#include "asset.h"
#include "utility/result.h"
#include <unordered_set>
#include <utility/asyncData.h>
#include <utility/asyncQueue.h>

class EntityManager;

class AssetLoader
{
  public:
    virtual AsyncData<Shared<Asset>> loadAsset(const AssetID& id, bool incremental) = 0;
    virtual ~AssetLoader() = default;
};

class AssetManager : public Module
{
  public:
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
        std::shared_ptr<Asset> asset;
        uint32_t useCount = 0;
        uint32_t unloadedDependencies = 0;
        LoadState loadState = LoadState::unloaded;
        std::unordered_set<AssetID> usedBy;
    };

  private:
    std::mutex _assetLock;
    std::unordered_map<AssetID, std::unique_ptr<AssetData>> _assets;
    std::unordered_map<AssetID, std::vector<std::function<void(Result<Shared<Asset>>)>>> _awaitingLoad;

    std::vector<std::unique_ptr<AssetLoader>> _loaders;

    size_t _nativeComponentID = 0;

    template<typename T>
    void addNativeComponent(EntityManager& em);

    void fetchAssetInternal(const AssetID& id,
                            bool incremental,
                            AssetData* entry,
                            AsyncData<Shared<Asset>> asset,
                            std::vector<std::unique_ptr<AssetLoader>>::iterator loader);
    AsyncData<Result<void>> loadDepsInternal(Shared<Asset> asset);
    void finalizeAssetInternal(Result<Shared<Asset>> result,
                               AssetData* entry,
                               const AssetID& id,
                               AsyncData<Shared<Asset>> promise);

  public:
    AssetManager();

    void addLoader(std::unique_ptr<AssetLoader> loader);

    template<typename T>
    Option<Shared<T>> getAsset(const AssetID& id)
    {
        static_assert(std::is_base_of<Asset, T>());
        std::scoped_lock lock(_assetLock);
        if(_assets.count(id))
            return Some<Shared<T>>(std::dynamic_pointer_cast<T>(std::shared_ptr<Asset>(_assets[id]->asset)));
        return None();
    }

    AsyncData<Shared<Asset>> fetchAsset(const AssetID& id, bool incremental = false);

    template<typename T>
    AsyncData<Shared<T>> fetchAsset(const AssetID& id)
    {
        static_assert(std::is_base_of<Asset, T>(), "fetchAsset requires T to inherit from Asset");
        AsyncData<Shared<T>> asset;
        fetchAsset(id, std::is_base_of<IncrementalAsset, T>())
            .then([asset](Shared<Asset> a) {
            asset.setData(std::move(std::dynamic_pointer_cast<T>(std::shared_ptr<Asset>(a))));
        }).onError([asset](std::string error) { asset.setError(error); });
        return asset;
    }

    void reloadAsset(Shared<Asset> asset);

    bool hasAsset(const AssetID& id);

    void fetchDependencies(Shared<Asset> asset, std::function<void(bool success)> callback);

    bool dependenciesLoaded(const Shared<Asset> asset) const;

    std::vector<Shared<Asset>> nativeAssets(AssetType type);

    static const char* name();

    void start() override;
};
