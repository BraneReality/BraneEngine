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

class LoginEvent : public GUIEvent
{
    net::Connection* _server;

  public:
    LoginEvent(net::Connection* server);

    inline net::Connection* server() const
    {
        return _server;
    }
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

#endif // BRANEENGINE_EDITOREVENTS_H
