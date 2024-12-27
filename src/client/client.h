#pragma once

#include "assets/asset.h"
#include "config/config.h"
#include "fileManager/fileManager.h"
#include "networking/networking.h"
#include "runtime/module.h"
#include "utility/threadPool.h"

class AssetManager;

namespace graphics
{
    class SceneRenderer;
}

class Client : public Module
{
    std::unordered_map<HashedAssetID, EntityID> _chunkRoots;
    graphics::SceneRenderer* _renderer;
    EntityID _mainCamera;

  public:
    Client();

    void start() override;

    static const char* name();
};
