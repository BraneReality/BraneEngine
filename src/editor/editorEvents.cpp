//
// Created by eli on 7/13/2022.
//

#include "editorEvents.h"

LoginEvent::LoginEvent(net::Connection* server) : GUIEvent("login")
{
    _server = server;
}

DirectoryUpdateEvent::DirectoryUpdateEvent(ServerDirectory* dir) : GUIEvent("dir reload")
{
    _dir = dir;
}
ServerDirectory* DirectoryUpdateEvent::directory() const
{
    return _dir;
}

FocusAssetEvent::FocusAssetEvent(const AssetID& asset) : GUIEvent("focus asset")
{
	_asset = asset;
}

const AssetID&FocusAssetEvent::asset() const
{
	return _asset;
}

FocusEntityAssetEvent::FocusEntityAssetEvent(size_t index) : GUIEvent("focus entity asset")
{
	_index = index;
}
size_t FocusEntityAssetEvent::entity() const
{
	return _index;
}

FocusEntityEvent::FocusEntityEvent(EntityID id) : GUIEvent("focus entity")
{
    _id = id;
}

EntityID FocusEntityEvent::id() const
{
    return _id;
}
