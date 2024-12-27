//
// Created by eli on 1/10/2022.
//

#include "database.h"
#include "assets/assetID.h"
#include "config/config.h"

#include "openssl/rand.h"
#include "utility/hex.h"

#define WIN32_LEAN_AND_MEAN

#include <iomanip>
#include "sqlite3.h"
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

Database::Database()
{
    int rc;
    rc = sqlite3_open(Config::json()["data"]["database_path"].asCString(), &_db);
    if(rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
        sqlite3_close(_db);
        throw std::runtime_error("Can't open database");
    }
    _getLastInserted.initialize("SELECT seq FROM sqlite_sequence WHERE name = ?1", _db);
    _loginCall.initialize("SELECT Logins.Password, Logins.Salt FROM Users "
                          "INNER JOIN Logins ON Logins.UserID = Users.UserID WHERE lower(Users.Username)=lower(?1);",
                          _db);
    _userIDCall.initialize("SELECT UserID FROM Users WHERE lower(Username)=lower(?1);", _db);

    _createUser.initialize("INSERT INTO Users (Username) VALUES (?1)", _db);
    _createPassword.initialize("INSERT INTO Logins (UserID, Password, Salt) VALUES (?1, ?2, ?3)", _db);
    _setPassword.initialize("UPDATE Logins SET Password=?2, Salt=?3 WHERE UserID = ?1", _db);

    _deleteUser.initialize("DELETE FROM Users WHERE UserID = ?1", _db);
    _deleteUserLogin.initialize("DELETE FROM Logins WHERE UserID = ?1", _db);
    _deleteUserPermissions.initialize("DELETE FROM UserPermissions WHERE UserID = ?1", _db);

    _getAssetInfo.initialize("SELECT AssetID, Name, Type, Hash FROM Assets WHERE AssetID = ?1", _db);
    _updateAssetInfo.initialize("UPDATE Assets SET Name = ?2, Type = ?3, Hash = ?4 WHERE AssetID = ?1", _db);
    _insertAssetInfo.initialize("INSERT INTO Assets (AssetID, Name, Type, Hash) VALUES (?1, ?2, ?3, ?4);", _db);
    _deleteAsset.initialize("DELETE FROM Assets WHERE AssetID = ?1", _db);

    _getAssetPermission.initialize("SELECT Level FROM AssetPermissions WHERE AssetID = ?1 AND UserID = ?2", _db);
    _updateAssetPermission.initialize("INSERT INTO AssetPermissions (AssetID, UserID, Level) VALUES (?1, ?2, ?3) "
                                      " ON CONFLICT DO UPDATE SET Level = ?3 WHERE AssetID = ?1 AND UserID = ?2",
                                      _db);
    _deleteAssetPermission.initialize("DELETE FROM AssetPermissions WHERE AssetID = ?1 AND UserID = ?2", _db);
    _searchAssets.initialize(
        "SELECT AssetID, Name, Type FROM Assets WHERE Name LIKE ?3 AND Type LIKE ?4 ORDER BY Name LIMIT ?2 OFFSET ?1",
        _db);
    _searchUsers.initialize(
        "SELECT UserID, Username FROM Users WHERE Username LIKE ?3 ORDER BY Username LIMIT ?2 OFFSET ?1", _db);

    rawSQLCall("SELECT * FROM Permissions;", [this](const std::vector<Database::sqlColumn>& columns) {
        _permissions.insert({std::stoi(columns[0].value), columns[1].value});
    });
}

Database::~Database() { sqlite3_close(_db); }

int Database::sqliteCallback(void* callback, int argc, char** argv, char** azColName)
{
    std::vector<sqlColumn> columns(argc);
    for(int i = 0; i < argc; ++i)
    {
        columns[i].name = azColName[i];
        columns[i].value = argv[i];
    }
    (*((sqlCallbackFunction*)(callback)))(columns);
    return 0;
}

void Database::rawSQLCall(const std::string& cmd, const sqlCallbackFunction& f)
{
    char* zErrMsg = 0;
    int rc;
    rc = sqlite3_exec(_db, cmd.c_str(), sqliteCallback, (void*)&f, &zErrMsg);
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

int64_t Database::getUserID(const std::string& username)
{
    int64_t userID;
    _userIDCall.run(username, std::function([&userID](int64_t id) { userID = id; }));
    return userID;
}

std::unordered_set<std::string> Database::userPermissions(int64_t userID)
{
    std::unordered_set<std::string> pems;
    // TODO change this to prepared call
    rawSQLCall("SELECT PermissionID FROM UserPermissions WHERE UserID=" + std::to_string(userID) + ";",
               [&pems, this](const std::vector<Database::sqlColumn>& columns) {
        pems.insert(_permissions[std::stoi(columns[0].value)]);
    });
    return pems;
}

void Database::insertAssetInfo(AssetInfo& assetInfo)
{
    _insertAssetInfo.run(assetInfo.id, sqlTEXT(assetInfo.name), assetInfo.type.toString(), assetInfo.hash);
    /*_getLastInserted.run("Assets", [&assetInfo](sqlINT id){
        assetInfo.id = id;
    });*/
}

AssetInfo Database::getAssetInfo(uint32_t id)
{
    AssetInfo asset{};
    _getAssetInfo.run(static_cast<sqlINT>(id), [&asset](sqlINT assetID, sqlTEXT name, sqlTEXT type, sqlTEXT hash) {
        asset.id = assetID;
        asset.name = std::move(name);
        asset.type.set(type);
        asset.hash = std::move(hash);
    });
    return asset;
}

void Database::updateAssetInfo(const AssetInfo& info)
{
    _updateAssetInfo.run(static_cast<sqlINT>(info.id), info.name, info.type.toString(), info.hash);
}

AssetPermissionLevel Database::getAssetPermission(uint32_t assetID, uint32_t userID)
{
    AssetPermissionLevel level = AssetPermissionLevel::none;
    _getAssetPermission.run(static_cast<sqlINT>(assetID), static_cast<sqlINT>(userID), [&level](sqlINT Level) {
        level = (AssetPermissionLevel)Level;
    });
    return level;
}

void Database::setAssetPermission(uint32_t assetID, uint32_t userID, AssetPermissionLevel level)
{
    if(level != AssetPermissionLevel::none)
        _updateAssetPermission.run(
            static_cast<sqlINT>(assetID), static_cast<sqlINT>(userID), static_cast<sqlINT>(level));
    else
        _deleteAssetPermission.run(static_cast<sqlINT>(assetID), static_cast<sqlINT>(userID));
}

std::vector<AssetInfo> Database::listUserAssets(const uint32_t& userID)
{
    std::string sqlCall = "SELECT Assets.* FROM Assets JOIN AssetPermissions ON Assets.AssetID = "
                          "AssetPermissions.AssetID WHERE AssetPermissions.UserID = " +
                          std::to_string(userID);
    std::vector<AssetInfo> assets;
    rawSQLCall(sqlCall, [&](const std::vector<Database::sqlColumn>& columns) {
        AssetInfo ai;
        ai.id = std::stoi(columns[0].value);
        ai.name = columns[1].value;
        ai.type.set(columns[2].value);
        assets.push_back(ai);
    });
    return assets;
}

std::string Database::assetName(AssetID& id)
{
    std::string name = "not found";
    std::string sqlCall = "SELECT Name FROM Assets WHERE AssetID = " + std::to_string(id.id());
    rawSQLCall(sqlCall, [&](const std::vector<Database::sqlColumn>& columns) { name = columns[0].value; });
    return name;
}

const char* Database::name() { return "database"; }

bool Database::authenticate(const std::string& username, const std::string& password)
{
    std::string passwordHashTarget;
    std::string salt;
    _loginCall.run(username, [&passwordHashTarget, &salt](std::string h, std::string s) {
        passwordHashTarget = h;
        salt = s;
    });
    if(passwordHashTarget.empty())
        return false;
    std::string hashedPassword = hashPassword(password, salt);

    return hashedPassword == passwordHashTarget;
}

std::string Database::hashPassword(const std::string& password, const std::string& salt)
{
    size_t hashIterations = Config::json()["security"].get("hash_iterations", 10000).asLargestInt();
    std::string input(SHA256_DIGEST_LENGTH, ' ');
    std::string output(SHA256_DIGEST_LENGTH, ' ');

    // Always do one iteration first to initialize input
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.size());
    SHA256_Update(&sha256, salt.c_str(), salt.size());
    SHA256_Final((unsigned char*)input.data(), &sha256);

    for(int i = 1; i < hashIterations; ++i)
    {
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, input.c_str(), input.size());
        SHA256_Update(&sha256, salt.c_str(), salt.size());
        SHA256_Final((unsigned char*)output.data(), &sha256);
        std::swap(input, output);
    }
    return output;
}

std::string Database::randHex(size_t length)
{
    std::string buffer(length, ' ');
    RAND_bytes((unsigned char*)buffer.data(), static_cast<int>(length));
    return std::move(buffer);
}

std::vector<Database::AssetSearchResult> Database::searchAssets(int start, int count, std::string match, AssetType type)
{
    std::vector<AssetSearchResult> results;
    if(count == 0)
        count = 1000;
    match = "%" + match + "%";
    std::string typeStr = (type == AssetType::none) ? "%" : type.toString();
    _searchAssets.run(start, count, match, typeStr, [&results](sqlINT id, sqlTEXT name, sqlTEXT type) {
        results.push_back(AssetSearchResult{static_cast<uint32_t>(id), std::move(name), AssetType::fromString(type)});
    });
    return results;
}

std::vector<Database::UserSearchResult> Database::searchUsers(int start, int count, std::string match)
{
    std::vector<UserSearchResult> results;
    if(count == 0)
        count = 1000;
    match = "%" + match + "%";
    _searchUsers.run(start, count, match, [&results](sqlINT id, sqlTEXT username) {
        results.push_back({(uint32_t)id, std::move(username)});
    });
    return results;
}

void Database::setPassword(uint32_t user, const std::string& password)
{
    std::string salt = randHex(SHA256_DIGEST_LENGTH);
    std::string hashedPassword = hashPassword(password, salt);
    _setPassword.run((int)user, hashedPassword, salt);
}

void Database::createUser(const std::string& username, const std::string& password)
{
    _createUser.run(username);
    uint32_t userID = 0;
    _getLastInserted.run("Users", [&userID](sqlINT id) { userID = id; });

    std::string salt = randHex(SHA256_DIGEST_LENGTH);
    std::string hashedPassword = hashPassword(password, salt);
    _createPassword.run((int)userID, hashedPassword, salt);
}

void Database::deleteUser(uint32_t user)
{
    _deleteUserPermissions.run((sqlINT)user);
    _deleteUserLogin.run((sqlINT)user);
    _deleteUser.run((sqlINT)user);
}
