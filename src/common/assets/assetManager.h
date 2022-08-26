#pragma once
#include <runtime/module.h>

#include <unordered_set>
#include <utility/asyncQueue.h>
#include <utility/asyncData.h>
#include "asset.h"

class EntityManager;
class AssetManager : public Module
{
public:
	using FetchCallback = std::function<AsyncData<Asset*>(const AssetID& id, bool incremental)>;

    enum class LoadState
    {
        unloaded,
        requested,
        awaitingDependencies,
        usable,
        loaded
    };

    struct AssetData
    {
        std::unique_ptr<Asset> asset;
        uint32_t useCount = 0;
        uint32_t unloadedDependencies = 0;
        LoadState loadState = LoadState::unloaded;
        std::unordered_set<AssetID> usedBy;
    };
private:
	std::mutex _assetLock;
	std::unordered_map<AssetID, std::unique_ptr<AssetData>> _assets;

    size_t _nativeComponentID = 0;
    template<typename T>
    void addNativeComponent(EntityManager& em);
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
	template <typename T>
	AsyncData<T*> fetchAsset(const AssetID& id)
	{
		static_assert(std::is_base_of<Asset, T>());
		AsyncData<T*> asset;
		fetchAsset(id, std::is_base_of<IncrementalAsset, T>()).then([asset](Asset* a)
		{
			asset.setData(std::move((T*)a));
		});
		return asset;
	}
	void addAsset(Asset* asset);
	bool hasAsset(const AssetID& id);


    bool dependenciesLoaded(const Asset* asset) const;
	void fetchDependencies(Asset* asset, const std::function<void()>& callback);

	static const char* name();
	void start() override;
};
