//
// Created by eli on 8/15/2022.
//

#ifndef BRANEENGINE_SYNCWINDOW_H
#define BRANEENGINE_SYNCWINDOW_H

#include <atomic>
#include "editorWindow.h"
#include "networking/connection.h"

class Asset;

class SyncWindow : public EditorWindow
{
    // Login variables
    static std::atomic_bool _loggedIn;
    static std::atomic_bool _loggingIn;
    std::string _serverAddress;
    std::string _port;
    std::string _username;
    std::string _password;
    static std::string _feedbackMessage;

    void drawSetupConnection();

    // sync variables
    static net::Connection* _syncServer;

    void drawConnected();

    void displayContent() override;

    struct AssetDiff
    {
        AssetID id;
    };

    std::vector<AssetDiff> _assetDiffs;
    std::atomic_int _assetDiffSynced = -1;

    void syncAssets();

    void updateAsset(const AssetID& asset);

    Json::Value _serverSettings;

    void serverSettings();

    std::atomic_int _usersSynced = -1;
    std::string _userFilter;

    struct UserInfo
    {
        uint32_t id;
        std::string name;
        std::set<std::string> permissions;
        bool synced;
    };

    std::vector<UserInfo> _users;
    UserInfo _newUser;
    std::string _newUserPassword;

    void drawUsers();

    void getUsers(const std::string& filter);

    static void displayLoadingAnim();

  public:
    SyncWindow(GUI& ui, Editor& editor);

    void refreshUsers();

    static net::Connection* syncServer();
};

#endif // BRANEENGINE_SYNCWINDOW_H
