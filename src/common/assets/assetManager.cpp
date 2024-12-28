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

const char* AssetManager::name() { return "assetManager"; }

template<typename T>
void AssetManager::addNativeComponent(EntityManager& em)
{
    static_assert(std::is_base_of<NativeComponent<T>, T>());
    ComponentDescription* description = T::constructDescription();
    ComponentAsset* asset = new ComponentAsset(
        T::getMemberTypes(), T::getMemberNames(), AssetID("native", static_cast<uint32_t>(_nativeComponentID++)));
    asset->name = T::getComponentName();
    asset->componentID = em.components().registerComponent(description);

    AssetData data{};
    data.asset = std::unique_ptr<Asset>(asset);
    data.loadState = LoadState::loaded;
    _assetLock.lock();
    _assets.insert({asset->id, std::make_unique<AssetData>(std::move(data))});
    _assetLock.unlock();
    description->asset = asset;
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

void AssetManager::fetchDependencies(Asset* a, std::function<void(bool)> callback)
{
    assert(a);
    assert(!a->id.null());
    std::vector<std::pair<AssetID, bool>> unloadedDeps;
    _assetLock.lock();
    for(AssetDependency& d : a->dependencies())
    {
        auto dep = _assets.find(d.id);
        if(dep == _assets.end() || dep->second->loadState < LoadState::usable)
        {
            std::pair<AssetID, bool> depPair = {d.id, d.streamable};
            // If the server address is empty, it means this asset is from the same origin as the parent.
            if(depPair.first.address().empty())
                depPair.first.setAddress(a->id.address());

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
            .then([this, id, callbackPtr](Asset* asset) {
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
            Runtime::error("Unable to fetch dependency of " + a->id.string() + ": " + message);
            (*callbackPtr)(false);
        });
    }
}

bool AssetManager::dependenciesLoaded(const Asset* asset) const
{
    auto deps = asset->dependencies();
    for(auto& d : deps)
        if(!_assets.count(d.id))
            return false;
    return true;
}

AsyncData<Asset*> AssetManager::fetchAsset(const AssetID& id, bool incremental)
{
    AsyncData<Asset*> asset;
    _assetLock.lock();
    if(_assets.count(id))
    {
        AssetData* assetData = _assets.at(id).get();
        if(assetData->loadState >= LoadState::usable)
            asset.setData(assetData->asset.get());
        else
            _awaitingLoad[id].push_back([asset](Asset* a) {
                if(a)
                    asset.setData(a);
                else
                    asset.setError("Asset failed to load");
            });
        _assetLock.unlock();
        return asset;
    }

    AssetData* assetData = new AssetData{};
    assetData->loadState = LoadState::requested;
    _assets.insert({id, std::unique_ptr<AssetData>(assetData)});

    _assetLock.unlock();
    fetchAssetInternal(id, incremental)
        .then([this, assetData, asset](Asset* a) {
        assert(a);
        _assetLock.lock();
        assetData->loadState = LoadState::loaded;
        assetData->asset = std::unique_ptr<Asset>(a);
        std::vector<std::function<void(Asset*)>> onLoaded;
        if(_awaitingLoad.count(a->id))
        {
            onLoaded = std::move(_awaitingLoad.at(a->id));
            _awaitingLoad.erase(a->id);
        }
        _assetLock.unlock();
        a->onDependenciesLoaded();
        for(auto& f : onLoaded)
            f(a);

        asset.setData(a);
    }).onError([this, asset, id](const std::string& error) {
        _assetLock.lock();
        std::vector<std::function<void(Asset*)>> onLoaded;
        if(_awaitingLoad.count(id))
        {
            onLoaded = std::move(_awaitingLoad.at(id));
            _awaitingLoad.erase(id);
        }
        _assets.erase(id);
        _assetLock.unlock();
        asset.setError(error);
        for(auto& f : onLoaded)
            f(nullptr);
    });

    return asset;
}

void AssetManager::reloadAsset(Asset* asset)
{
    assert(asset);
    if(!hasAsset(asset->id))
        return;
    std::scoped_lock lock(_assetLock);

    // We move instead of just replacing the pointer to avoid breaking references
    switch(asset->type.type())
    {
        case AssetType::mesh:
            *(MeshAsset*)(_assets.at(asset->id)->asset.get()) = std::move(*(MeshAsset*)(asset));
            break;
        case AssetType::shader:
            *(ShaderAsset*)(_assets.at(asset->id)->asset.get()) = std::move(*(ShaderAsset*)(asset));
            break;
        case AssetType::material:
            *(MaterialAsset*)(_assets.at(asset->id)->asset.get()) = std::move(*(MaterialAsset*)(asset));
            break;
        case AssetType::assembly:
            *(Assembly*)(_assets.at(asset->id)->asset.get()) = std::move(*(Assembly*)(asset));
            break;
        default:
            Runtime::warn("Assembly manager attempted to reload asset of type " + asset->type.toString() +
                          " but currently it isn't supported");
    }
}

std::vector<const Asset*> AssetManager::nativeAssets(AssetType type)
{
    std::scoped_lock lock(_assetLock);
    std::vector<const Asset*> assets;
    switch(type.type())
    {
        case AssetType::none:
            break;
        case AssetType::component:
            assets = {EntityIDComponent::def()->asset,
                      EntityName::def()->asset,
                      Transform::def()->asset,
                      LocalTransform::def()->asset,
                      Children::def()->asset,
                      TRS::def()->asset,
                      MeshRendererComponent::def()->asset,
                      PointLightComponent::def()->asset};
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
