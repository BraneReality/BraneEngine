//
// Created by eli on 7/13/2022.
//

#ifndef BRANEENGINE_EDITOREVENTS_H
#define BRANEENGINE_EDITOREVENTS_H

#include <cstdint>
#include <memory>
#include "assets/assetID.h"
#include "common/ui/guiEvent.h"
#include "ecs/entityID.h"

namespace net
{
    class Connection;
}
class ServerDirectory;

class LoginEvent : public GUIEvent
{
    net::Connection* _server;

  public:
    LoginEvent(net::Connection* server);

    inline net::Connection* server() const { return _server; }
};

class DirectoryUpdateEvent : public GUIEvent
{
    ServerDirectory* _dir;

  public:
    DirectoryUpdateEvent(ServerDirectory* dir);

    ServerDirectory* directory() const;
};

class AssetReloadEvent : public GUIEvent
{
  public:
    AssetReloadEvent();
};

class EntityAssetReloadEvent : public GUIEvent
{
    size_t _entity;

  public:
    EntityAssetReloadEvent(size_t entity);

    size_t entity() const;
};

class EditorAsset;

class FocusAssetEvent : public GUIEvent
{
    std::shared_ptr<EditorAsset> _asset;

  public:
    FocusAssetEvent(std::shared_ptr<EditorAsset> asset);

    std::shared_ptr<EditorAsset> asset() const;
};

class FocusEntityAssetEvent : public GUIEvent
{
    int _index;

  public:
    FocusEntityAssetEvent(int index);

    int entity() const;
};

class FocusEntityEvent : public GUIEvent
{
    EntityID _id;

  public:
    FocusEntityEvent(EntityID id);

    EntityID id() const;
};

#endif // BRANEENGINE_EDITOREVENTS_H
