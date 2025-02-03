//
// Created by eli on 7/13/2022.
//

#include "editorEvents.h"
#include "fileManager/fileManager.h"

LoginEvent::LoginEvent(net::Connection* server) : GUIEvent("login")
{
    _server = server;
}

EntityAssetReloadEvent::EntityAssetReloadEvent(size_t entity) : GUIEvent("entity asset reload")
{
    _entity = entity;
}

size_t EntityAssetReloadEvent::entity() const
{
    return _entity;
}

AssetReloadEvent::AssetReloadEvent() : GUIEvent("asset reload") {}
