#include "client.h"
#include "runtime/runtime.h"

#include "assets/assembly.h"
#include "assets/assetManager.h"
#include "ecs/nativeTypes/assetComponents.h"
#include "ecs/nativeTypes/meshRenderer.h"
#include "graphics/camera.h"
#include "graphics/graphics.h"
#include "graphics/sceneRenderer.h"
#include "networking/networking.h"
#include "systems/transforms.h"

#include <asio.hpp>
#include "assets/chunk.h"
#include "chunk/chunkLoader.h"
#include "glm/gtx/quaternion.hpp"
#include "networking/connection.h"
#include <utility/threadPool.h>

class ClientAssetLoader : public AssetLoader
{
    AsyncData<Asset*> loadAsset(const AssetID& inId, bool incremental) override
    {
        AsyncData<Asset*> asset;
        auto castId = inId.as<BraneAssetID>();
        if(!castId)
        {
            asset.setError("Can only load BraneAssetID");
            return asset;
        }
        auto* id = castId.value();
        if(id->domain.empty())
        {
            asset.setError("Asset with id " + id->toString() +
                           " was not found and can not be remotely fetched since it lacks a server address");
            return asset;
        }
        auto* nm = Runtime::getModule<NetworkManager>();
        if(incremental)
        {
            nm->async_requestAssetIncremental(*id).then([this, asset](Asset* ptr) {
                asset.setData(ptr);
            }).onError([this, asset](std::string error) { asset.setError(error); });
        }
        else
        {
            nm->async_requestAsset(*id).then([this, asset](Asset* ptr) {
                asset.setData(ptr);
            }).onError([this, asset](std::string error) { asset.setError(error); });
        }
        return asset;
    }
};

const char* Client::name()

{
    return "client";
}

Client::Client() {}

void Client::start()
{
    auto* nm = Runtime::getModule<NetworkManager>();
    auto* am = Runtime::getModule<AssetManager>();
    auto* cl = Runtime::getModule<ChunkLoader>();
    auto* em = Runtime::getModule<EntityManager>();

    am->addLoader(std::make_unique<ClientAssetLoader>());

    cl->addOnLODChangeCallback([this, em, am](const WorldChunk* chunk, uint32_t oldLod, uint32_t newLod) {
        Runtime::error("LOD changes not implemented");
        return;
        /*
        if(oldLod != NullLOD)
        {
            for(auto& lod : chunk->LODs)
            {
                if(lod.min > newLod || lod.max < newLod)
                {
                    assert(_chunkRoots.contains(lod.assembly));
                    Runtime::getModule<Transforms>()->destroyRecursive(_chunkRoots.at(lod.assembly));
                    _chunkRoots.erase(lod.assembly);
                }
            }
        }
        if(newLod != NullLOD)
        {
            for(auto& lod : chunk->LODs)
            {
                if(lod.min <= newLod && newLod <= lod.max)
                {
                    AssetID id = lod.assembly;
                    if(id.domain.empty())
                        id.domain = chunk->id.domain;
                    am->fetchAsset<Assembly>(id).thenMain(
                        [this, em](Assembly* assembly) { _chunkRoots[assembly->id] = assembly->inject(*em); });
                }
            }
        }
        */
    });

    nm->addRequestListener("loadChunk", [am](auto& rc) {
        AssetID chunkID;
        rc.req >> chunkID;
        Runtime::log("Was requested to load chunk " + chunkID.toString());
        am->fetchAsset<WorldChunk>(chunkID).then([](WorldChunk* chunk) {
            Runtime::getModule<ChunkLoader>()->loadChunk(chunk);
            Runtime::log("Loaded chunk " + chunk->id.toString());
        });
    });
    nm->addRequestListener("unloadChunk", [](auto& rc) { Runtime::log("Was requested to unload chunk"); });

    auto* vkr = Runtime::getModule<graphics::VulkanRuntime>();
    _renderer = vkr->createRenderer<graphics::SceneRenderer>(vkr, em);
    _renderer->setClearColor({.2, .2, .2, 1});
    _renderer->setTargetAsSwapChain(true);

    _mainCamera = em->createEntity(ComponentSet{{Transform::def()->id, TRS::def()->id, graphics::Camera::def()->id}});
    TRS cameraTransform;
    cameraTransform.translation = {4, 2, -4};
    cameraTransform.rotation = glm::quatLookAt(glm::normalize(glm::vec3{2, 1, -2}), {0, 1, 0});
    em->setComponent(_mainCamera, cameraTransform);
    graphics::Camera camera;
    camera.fov = 45;
    em->setComponent(_mainCamera, camera);

    nm->async_connectToAssetServer("localhost", 2001, [nm](bool success) {
        if(success)
        {
            auto* assetServer = nm->getServer("localhost");
            SerializedData data;
            OutputSerializer s(data);
            std::string username = Config::json()["user"]["username"].asString();
            std::string password = Config::json()["user"]["password"].asString();
            s << username << password;
            assetServer->sendRequest("login", std::move(data), [assetServer](auto rc, InputSerializer res) {
                if(rc == net::ResponseCode::success)
                {
                    assetServer->sendRequest("defaultChunk", {}, [](auto rc, InputSerializer res) {
                        if(rc != net::ResponseCode::success)
                        {
                            Runtime::error("Problem fetching default chunk");
                            return;
                        }
                        Runtime::log("Requested default chunk");
                    });
                }
                else
                {
                    Runtime::error("Failed to log in");
                }
            });
        }
        else
        {
            Runtime::error("Failed to connect to asset server");
        }
    });
}
