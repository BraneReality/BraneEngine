#include "assetManager.h"
#include "assembly.h"
#include "ecs/entity.h"
#include "ecs/nativeComponent.h"
#include "ecs/nativeTypes/meshRenderer.h"
#include "graphics/camera.h"
#include "graphics/pointLightComponent.h"
#include "systems/transforms.h"
#include "types/materialAsset.h"
#include "types/meshAsset.h"
#include "types/shaderAsset.h"
#include "utility/variantMatch.h"

AssetManager::AssetManager()
{
    EntityManager* em = Runtime::getModule<EntityManager>();
    assert(em);
    addNativeComponent<EntityIDComponent>(*em);
    addNativeComponent<EntityName>(*em);
    addNativeComponent<Transform>(*em);
    addNativeComponent<LocalTransform>(*em);
    addNativeComponent<Children>(*em);
    addNativeComponent<TRS>(*em);
    addNativeComponent<MeshRendererComponent>(*em);
    addNativeComponent<PointLightComponent>(*em);
    addNativeComponent<graphics::Camera>(*em);
}

void AssetManager::addLoader(std::unique_ptr<AssetLoader> loader)
{
    _loaders.push_back(std::move(loader));
}

const char* AssetManager::name()
{
    return "assetManager";
}

template<typename T>
void AssetManager::addNativeComponent(EntityManager& em)
{
    static_assert(std::is_base_of<NativeComponent<T>, T>());
    ComponentDescription* description = T::constructDescription();
    std::shared_ptr<ComponentAsset> asset = std::shared_ptr<ComponentAsset>(
        new ComponentAsset(T::getMemberTypes(),
                           T::getMemberNames(),
                           FileAssetID(std::format("native/{}", static_cast<uint32_t>(_nativeComponentID++)))));
    asset->name = T::getComponentName();
    asset->componentID = em.components().registerComponent(description);

    AssetData data{
        .asset = std::static_pointer_cast<Asset>(asset),
    };
    data.loadState = LoadState::loaded;
    _assetLock.lock();
    _assets.insert({asset->id, std::make_unique<AssetData>(std::move(data))});
    _assetLock.unlock();
    description->asset = asset.get();
}

void AssetManager::start() {}

bool AssetManager::hasAsset(const AssetID& id)
{
    std::scoped_lock lock(_assetLock);
    if(!_assets.count(id))
        return false;
    auto state = _assets.at(id)->loadState;
    return (uint8_t)state >= (uint8_t)LoadState::awaitingDependencies;
}

void AssetManager::fetchDependencies(Shared<Asset> a, std::function<void(bool)> callback)
{
    assert(a.get());
    assert(!a->id.empty());
    std::vector<std::pair<AssetID, bool>> unloadedDeps;
    _assetLock.lock();
    for(AssetDependency& d : a->dependencies())
    {
        auto dep = _assets.find(d.id);
        if(dep == _assets.end() || dep->second->loadState < LoadState::usable)
        {
            std::pair<AssetID, bool> depPair = {d.id, d.streamable};
            unloadedDeps.push_back(std::move(depPair));
        }
    }
    _assetLock.unlock();
    if(unloadedDeps.empty())
    {
        callback(true);
        return;
    }

    auto callbackPtr = std::make_shared<std::function<void(bool)>>(std::move(callback));
    _assetLock.lock();
    AssetData* data = _assets.at(a->id).get();
    data->unloadedDependencies = unloadedDeps.size();
    _assetLock.unlock();
    AssetID id = a->id;
    for(auto& d : unloadedDeps)
    {
        fetchAsset(d.first, d.second)
            .then([this, id, callbackPtr](Shared<Asset> asset) {
            Runtime::log("Loaded: " + asset->name);
            _assetLock.lock();
            if(!_assets.count(id))
            {
                _assetLock.unlock();
                return;
            }
            auto* data = _assets.at(id).get();
            auto remaining = --data->unloadedDependencies;
            _assetLock.unlock();
            if(remaining == 0)
                (*callbackPtr)(true);
        }).onError([a, callbackPtr](const std::string& message) {
            Runtime::error("Unable to fetch dependency of " + a->id.toString() + ": " + message);
            (*callbackPtr)(false);
        });
    }
}

bool AssetManager::dependenciesLoaded(const Shared<Asset> asset) const
{
    auto deps = asset->dependencies();
    for(auto& d : deps)
        if(!_assets.count(d.id))
            return false;
    return true;
}

AsyncData<Result<void>> AssetManager::loadDepsInternal(Shared<Asset> asset)
{
    AsyncData<Result<void>> promise;
    _assetLock.lock();
    _assets.at(asset->id)->loadState = LoadState::awaitingDependencies;
    _assetLock.unlock();
    if(dependenciesLoaded(asset))
    {
        promise.setData(Ok<void>());
        return promise;
    }
    fetchDependencies(asset, [asset, promise](bool success) {
        if(success)
            promise.setData(Ok<void>());
        else
            promise.setError("Failed to load dependency for: " + asset->name);
    });
    return promise;
}

void AssetManager::finalizeAssetInternal(Result<Shared<Asset>> result,
                                         AssetData* entry,
                                         const AssetID& id,
                                         AsyncData<Shared<Asset>> promise)
{
    if(!result)
    {
        auto error = result.err();
        _assetLock.lock();
        std::vector<std::function<void(Result<Shared<Asset>>)>> onLoaded;
        if(_awaitingLoad.count(id))
        {
            onLoaded = std::move(_awaitingLoad.at(id));
            _awaitingLoad.erase(id);
        }
        _assets.erase(id);
        _assetLock.unlock();

        promise.setError(error);
        for(auto& f : onLoaded)
            f(Err(error));
        return;
    }
    Shared<Asset> a = result.ok();
    loadDepsInternal(a)
        .then([this, a, entry, promise](Result<void> depsResult) {
        _assetLock.lock();
        entry->loadState = LoadState::loaded;
        entry->asset = a;
        std::vector<std::function<void(Result<Shared<Asset>>)>> onLoaded;
        if(_awaitingLoad.count(a->id))
        {
            onLoaded = std::move(_awaitingLoad.at(a->id));
            _awaitingLoad.erase(a->id);
        }
        _assetLock.unlock();
        entry->asset->onDependenciesLoaded();
        for(auto& f : onLoaded)
            f(Ok(a));

        promise.setData(a);
    }).onError([this, a, entry, id, promise](std::string error) {
        finalizeAssetInternal(Err(error), entry, id, promise);
    });
}

void AssetManager::fetchAssetInternal(const AssetID& id,
                                      bool incremental,
                                      AssetData* entry,
                                      AsyncData<Shared<Asset>> asset,
                                      std::vector<std::unique_ptr<AssetLoader>>::iterator loader)
{
    if(loader == _loaders.end())
    {
        finalizeAssetInternal(Err(std::format("Asset {} could not be fetched", id.toString())), entry, id, asset);
        return;
    }
    (*loader)
        ->loadAsset(id, incremental)
        .then([this, entry, asset, id](Shared<Asset> a) {
        finalizeAssetInternal(Ok(a), entry, id, asset);
    }).onError([this, entry, asset, id](const std::string& error) {
        finalizeAssetInternal(Err(error), entry, id, asset);
    });
}

AsyncData<Shared<Asset>> AssetManager::fetchAsset(const AssetID& id, bool incremental)
{
    AsyncData<Shared<Asset>> asset;
    Runtime::log("Fetching asset " + id.toString());
    _assetLock.lock();
    for(auto& assetData : _assets)
    {
        Runtime::log(std::format(
            "We have data for {} is match = {}", assetData.first.toString(), assetData.first == id ? "true" : "false"));
    }
    if(_assets.count(id))
    {
        AssetData* assetData = _assets.at(id).get();
        if(assetData->loadState >= LoadState::usable)
            asset.setData(assetData->asset);
        else
            _awaitingLoad[id].push_back([asset](Result<Shared<Asset>> a) {
                if(a)
                    asset.setData(a.ok());
                else
                    asset.setError(a.err());
            });
        _assetLock.unlock();
        return asset;
    }

    auto assetData = std::make_unique<AssetData>();
    assetData->loadState = LoadState::requested;
    auto assetDataRef = assetData.get();

    _assets.insert({id, std::move(assetData)});

    _assetLock.unlock();

    if(_loaders.empty())
    {
        asset.setError("No asset loaders set!");
        return asset;
    }
    fetchAssetInternal(id, incremental, assetDataRef, asset, _loaders.begin());

    return asset;
}

void AssetManager::reloadAsset(Shared<Asset> asset)
{
    if(!hasAsset(asset->id))
        return;
    std::scoped_lock lock(_assetLock);

    // We move instead of just replacing the pointer to avoid breaking references
    switch(asset->type.type())
    {
        case AssetType::mesh:
            *(MeshAsset*)(_assets.at(asset->id)->asset.get()) = std::move(*(MeshAsset*)(asset.get()));
            break;
        case AssetType::shader:
            *(ShaderAsset*)(_assets.at(asset->id)->asset.get()) = std::move(*(ShaderAsset*)(asset.get()));
            break;
        case AssetType::material:
            *(MaterialAsset*)(_assets.at(asset->id)->asset.get()) = std::move(*(MaterialAsset*)(asset.get()));
            break;
        case AssetType::assembly:
            *(Assembly*)(_assets.at(asset->id)->asset.get()) = std::move(*(Assembly*)(asset.get()));
            break;
        default:
            Runtime::warn("Assembly manager attempted to reload asset of type " + asset->type.toString() +
                          " but currently it isn't supported");
    }
}

std::vector<Shared<Asset>> AssetManager::nativeAssets(AssetType type)
{
    std::scoped_lock lock(_assetLock);
    std::vector<Shared<Asset>> assets;
    switch(type.type())
    {
        case AssetType::none:
            break;
        case AssetType::component:
            assets = {std::shared_ptr<Asset>((Asset*)EntityIDComponent::def()->asset),
                      std::shared_ptr<Asset>((Asset*)EntityName::def()->asset),
                      std::shared_ptr<Asset>((Asset*)Transform::def()->asset),
                      std::shared_ptr<Asset>((Asset*)LocalTransform::def()->asset),
                      std::shared_ptr<Asset>((Asset*)Children::def()->asset),
                      std::shared_ptr<Asset>((Asset*)TRS::def()->asset),
                      std::shared_ptr<Asset>((Asset*)MeshRendererComponent::def()->asset),
                      std::shared_ptr<Asset>((Asset*)PointLightComponent::def()->asset)};
            break;
        case AssetType::script:
            break;
        case AssetType::mesh:
            break;
        case AssetType::image:
            break;
        case AssetType::shader:
            break;
        case AssetType::material:
            break;
        case AssetType::assembly:
            break;
        case AssetType::chunk:
            break;
        case AssetType::player:
            break;
    }
    return assets;
}
