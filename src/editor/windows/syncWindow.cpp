//
// Created by eli on 8/15/2022.
//

#include "syncWindow.h"
#include "../widgets/assetSelectWidget.h"
#include "assets/asset.h"
#include "assets/assetManager.h"
#include "editor/assets/editorAsset.h"
#include "editor/editor.h"
#include "editor/editorEvents.h"
#include "imgui_stdlib.h"
#include "networking/networking.h"
#include "ui/gui.h"
#include "ui/guiPopup.h"
#include "ui/IconsFontAwesome6.h"
#include "utility/threadPool.h"

std::atomic_bool SyncWindow::_loggedIn = false;
std::atomic_bool SyncWindow::_loggingIn = false;
net::Connection* SyncWindow::_syncServer = nullptr;
std::string SyncWindow::_feedbackMessage;

SyncWindow::SyncWindow(GUI& ui, Editor& editor) : EditorWindow(ui, editor)
{
    _name = "Sync";

    auto& project = _editor.project().value()->data();
    _serverAddress = *project.assetServer->address->value();
    _port = *project.assetServer->port->value();
    _username = "";
}

void SyncWindow::displayContent()
{
    if(_loggedIn)
        drawConnected();
    else
        drawSetupConnection();
}

void SyncWindow::drawSetupConnection()
{
    ImGui::Text("Select Server:");
    ImGui::InputText("address", &_serverAddress);
    ImGui::InputText("port", &_port);
    ImGui::Separator();
    ImGui::Text("Login:");
    ImGui::InputText("username", &_username);
    ImGui::InputText("password", &_password, ImGuiInputTextFlags_Password);
    bool enterPressed = ImGui::IsKeyReleased(ImGuiKey_Enter) && ImGui::IsItemFocused();

    ImGui::Separator();

    bool disableButton = _loggingIn;
    if(disableButton)
        ImGui::BeginDisabled();
    if((ImGui::Button("Submit") || enterPressed) && !_loggingIn && !_loggedIn)
    {
        _loggingIn = true;
        _feedbackMessage = "Connecting...";
        NetworkManager* nm = Runtime::getModule<NetworkManager>();
        auto port = std::stoi(_port);
        nm->async_connectToAssetServer(_serverAddress, port, [this, port, nm](bool success) {
            if(success)
            {
                _feedbackMessage = "Connected, logging in...";
                SerializedData req;
                OutputSerializer s(req);
                s << _username << _password;

                auto* server = nm->getServer(_serverAddress);
                server->sendRequest(
                    "login", std::move(req), [this, port, server](net::ResponseCode code, InputSerializer sData) {
                    if(code != net::ResponseCode::success)
                    {
                        _loggingIn = false;
                        if(code == net::ResponseCode::denied)
                            _feedbackMessage = "invalid username/password";
                        else
                            _feedbackMessage = "request error: " + std::to_string((uint8_t)code);
                        return;
                    }

                    _feedbackMessage = "Logged in!";
                    _loggedIn = true;
                    _loggingIn = false;

                    auto& project = _editor.project().value()->data();
                    project.assetServer->address->set(_serverAddress).forward();
                    project.assetServer->port->set(port).forward();

                    _syncServer = server;
                    _syncServer->onDisconnect([]() {
                        // Don't need a "this" pointer as they're static
                        _loggedIn = false;
                        _loggingIn = false;
                        _syncServer = nullptr;
                        _feedbackMessage = "Disconnected from server!";
                    });
                });
            }
            else
            {
                _loggedIn = false;
                _loggingIn = false;
                _feedbackMessage = "Unable to connect to server";
            }
        });
    }
    if(disableButton)
        ImGui::EndDisabled();
    ImGui::Text("%s", _feedbackMessage.c_str());
}

net::Connection* SyncWindow::syncServer()
{
    return _syncServer;
}

void SyncWindow::drawConnected()
{
    if(ImGui::BeginTabBar("SubWindows"))
    {
        if(ImGui::BeginTabItem("Sync Assets"))
        {
            syncAssets();
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Server Settings"))
        {
            serverSettings();
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Users"))
        {
            drawUsers();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void SyncWindow::syncAssets()
{
    if(_assetDiffSynced == -1)
    {
        ThreadPool::enqueue([this] {
            SerializedData assetHashes;
            OutputSerializer s(assetHashes);
            std::vector<std::pair<AssetID, std::string>> hashes; // = _editor.project().getAssetHashes();
            Runtime::warn("Syncing broken right now");

            uint32_t diffs = hashes.size();
            s << diffs;
            for(auto& h : hashes)
                s << h.first << h.second;

            _syncServer->sendRequest("getAssetDiff", std::move(assetHashes), [this](auto code, InputSerializer res) {
                if(code != net::ResponseCode::success)
                {
                    if(code == net::ResponseCode::denied)
                    {
                        Runtime::error("Could not retrieve asset differences! (Invalid Permissions)");
                    }
                    else
                    {
                        Runtime::error("Could not retrieve asset differences!");
                    }
                    return;
                }
                uint32_t diffs;
                res >> diffs;
                _assetDiffs.resize(diffs);
                for(uint32_t i = 0; i < diffs; ++i)
                    res >> _assetDiffs[i].id;
                _assetDiffSynced = 1;
            });
        });
        _assetDiffSynced = 0;
    }
    if(_assetDiffSynced == 0)
    {
        displayLoadingAnim();
        return;
    }

    ImGui::Text("Changed Assets: ");
    ImGui::SameLine(ImGui::GetContentRegionMax().x - 68);
    if(ImGui::Button("Upload All " ICON_FA_CLOUD_ARROW_UP))
    {
        for(auto& asset : _assetDiffs)
            updateAsset(asset.id);
        _assetDiffSynced = -1;
    }
    for(auto& asset : _assetDiffs)
    {
        ImGui::PushID(&asset.id);
        ImGui::Text("%s: %s", asset.id.toString().c_str(), asset.id.toString().c_str());
        ImGui::SameLine(ImGui::GetContentRegionMax().x - 20);
        if(ImGui::Button(ICON_FA_CLOUD_ARROW_UP))
        {
            updateAsset(asset.id);
            _assetDiffSynced = -1;
        }
        if(ImGui::IsItemHovered())
            ImGui::SetTooltip("Sync Asset");
        ImGui::PopID();
    }
    if(ImGui::Button("Resync " ICON_FA_ARROWS_ROTATE))
        _assetDiffSynced = -1;
}

void SyncWindow::updateAsset(const AssetID& asset)
{
    SerializedData assetData;
    OutputSerializer s(assetData);

    if(_editor.cache().hasAsset(asset))
        _editor.cache().getAsset(asset)->serialize(s);
    else
    {
        auto buildRes = _editor.project().value()->getEditorAsset(asset).value()->buildAsset(asset);
        if(!buildRes)
        {
            Runtime::error(std::format("Failed to build {}", asset.toString()));
            return;
        }
        auto a = buildRes.ok();
        a->serialize(s);
        _editor.cache().cacheAsset(a.get());
    }
    _syncServer->sendRequest("updateAsset", std::move(assetData), [asset](auto ec, InputSerializer res) {
        if(ec == net::ResponseCode::success)
            Runtime::log("Asset " + asset.toString() + " updated");
        else
            Runtime::error("Failed to update asset " + asset.toString());
    });
}

void SyncWindow::serverSettings()
{
    if(_serverSettings.empty())
    {
        _serverSettings["loading"] = true;
        _syncServer->sendRequest("getServerSettings", {}, [this](auto ec, InputSerializer res) {
            if(ec != net::ResponseCode::success)
            {
                if(ec == net::ResponseCode::denied)
                {
                    Runtime::error("Could not load server settings! (Invalid Permissions)");
                }
                else
                {
                    Runtime::error("Could not load server settings!");
                }
            }
            else
            {
                res >> _serverSettings;
            }
        });
    }

    if(_serverSettings.isMember("loading"))
    {
        displayLoadingAnim();
        return;
    }

    AssetID defaultChunk = AssetID::parse(_serverSettings["default_assets"]["chunk"].asString()).ok();
    if(AssetSelectWidget::draw(defaultChunk, AssetType::chunk))
        _serverSettings["default_assets"]["chunk"] = defaultChunk.toString();

    if(ImGui::Button("Save " ICON_FA_CLOUD_ARROW_UP))
    {
        SerializedData data;
        OutputSerializer s(data);
        s << _serverSettings;

        _syncServer->sendRequest("setServerSettings", std::move(data), [](auto ec, InputSerializer res) {
            if(ec == net::ResponseCode::success)
                Runtime::log("Server settings updated");
            else
                Runtime::error("Failed to update server settings");
        });
    }
}

class DeleteUserPopup : public GUIPopup
{
    std::string _username;
    uint32_t _userID;
    uint32_t _clicks = 4;
    SyncWindow& _window;

    void drawBody() override
    {
        ImGui::Text("Remove %s?", _username.c_str());
        if(ImGui::Button("Yes"))
            --_clicks;
        ImGui::SameLine();
        if(ImGui::Button("No"))
            ImGui::CloseCurrentPopup();
        ImGui::TextDisabled("%u clicks remaining", _clicks);
        if(_clicks == 0)
        {
            SerializedData data;
            OutputSerializer s(data);
            s << _userID;
            std::string username = _username;
            auto& win = _window;
            _window.syncServer()->sendRequest(
                "adminDeleteUser", std::move(data), [username, &win](auto ec, InputSerializer s) {
                if(ec != net::ResponseCode::success)
                    Runtime::log("Couldn't delete " + username);
                win.refreshUsers();
            });
            ImGui::CloseCurrentPopup();
        }
    }

  public:
    DeleteUserPopup(std::string username, uint32_t userID, SyncWindow& window)
        : _username(std::move(username)), _userID(userID), _window(window), GUIPopup("Delete User") {};
};

class EditPasswordPopup : public GUIPopup
{
    net::Connection* _server;
    uint32_t _userID;
    std::string _newPassword1;
    std::string _newPassword2;

    std::string _result;

    void drawBody() override
    {
        ImGui::Text("Change Password");
        ImGui::InputText("New password", &_newPassword1, ImGuiInputTextFlags_Password);
        ImGui::InputText("Repeat password", &_newPassword2, ImGuiInputTextFlags_Password);
        bool valid = true;
        if(_newPassword1.size() < 8)
        {
            ImGui::TextDisabled("Password must be at least 8 characters long");
            valid = false;
        }
        else if(_newPassword1 != _newPassword2)
        {
            ImGui::TextDisabled("Passwords do not match");
            valid = false;
        }
        ImGui::BeginDisabled(!valid);
        if(ImGui::Button("Save"))
        {
            SerializedData data;
            OutputSerializer s(data);
            s << _userID << _newPassword1;
            _result = "saving";
            _server->sendRequest("adminChangePassword", std::move(data), [this](auto rc, InputSerializer s) {
                switch(rc)
                {
                    case net::ResponseCode::success:
                        _result = "success!";
                        break;
                    case net::ResponseCode::denied:
                        _result = "access denied";
                        break;
                    default:
                        _result = "error";
                }
            });
        }
        ImGui::EndDisabled();
        if(!_result.empty())
            ImGui::Text("%s", _result.c_str());
    }

  public:
    EditPasswordPopup(uint32_t userID, net::Connection* server)
        : _userID(userID), _server(server), GUIPopup("Edit Password") {};
};

void SyncWindow::drawUsers()
{
    _ui.pushHeaderFont();
    ImGui::Text("Users");
    ImGui::PopFont();
    if(ImGui::InputText("Search", &_userFilter))
    {
        _usersSynced = -1;
    }

    if(_usersSynced == -1)
    {
        getUsers(_userFilter);
    }

    ImGui::BeginChild("Users", {0, std::max(400.0f, ImGui::GetContentRegionAvail().y / 2.0f)});
    if(_usersSynced == 1)
    {
        for(auto& user : _users)
        {
            if(ImGui::CollapsingHeader(user.name.c_str()))
            {
                ImGui::Indent();
                ImGui::PushID(user.id);
                ImGui::TextDisabled("id: %u", user.id);
                ImGui::InputText("username", &user.name);
                if(ImGui::Button("Change Password"))
                    _ui.openPopup(std::make_unique<EditPasswordPopup>(user.id, _syncServer));
                if(ImGui::Button("Save"))
                {
                    Runtime::warn("Changing user data not supported yet");
                }
                ImGui::SameLine();
                if(ImGui::Button("Delete"))
                    _ui.openPopup(std::make_unique<DeleteUserPopup>(user.name, user.id, *this));
                ImGui::PopID();
                ImGui::Unindent();
            }
        }
    }
    ImGui::EndChild();
    ImGui::Separator();
    _ui.pushHeaderFont();
    ImGui::Text("Create User");
    ImGui::PopFont();
    ImGui::InputText("Username", &_newUser.name);
    ImGui::InputText("Password", &_newUserPassword);
    if(ImGui::Button("Create"))
    {
        SerializedData data;
        OutputSerializer s(data);
        s << _newUser.name;
        s << _newUserPassword;
        _syncServer->sendRequest("newUser", std::move(data), [this](auto ec, InputSerializer s) {
            if(ec == net::ResponseCode::success)
            {
                Runtime::log("User " + _newUser.name + " created!");
                _usersSynced = -1;
            }
        });
    }
}

void SyncWindow::getUsers(const std::string& filter)
{
    SerializedData data;
    OutputSerializer s(data);
    s << filter;
    _syncServer->sendRequest("searchUsers", std::move(data), [this](auto ec, InputSerializer res) {
        if(ec != net::ResponseCode::success)
        {
            Runtime::error("Couldn't search users, error: " + std::to_string((uint8_t)ec));
            return;
        }
        uint32_t userCount;
        res >> userCount;
        _users.clear();
        _users.resize(userCount);
        for(uint32_t i = 0; i < userCount; ++i)
            res >> _users[i].id >> _users[i].name;

        _usersSynced = 1;
    });

    _usersSynced = 0;
}

void SyncWindow::refreshUsers()
{
    _usersSynced = -1;
}

void SyncWindow::displayLoadingAnim()
{
    const char* dots[] = {"", ".", "..", "..."};
    size_t time =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() %
        4;
    ImGui::Text("Loading%s", dots[time]);
}
