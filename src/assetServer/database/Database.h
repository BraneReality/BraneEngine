//
// Created by eli on 1/10/2022.
//

#ifndef BRANEENGINE_DATABASE_H
#define BRANEENGINE_DATABASE_H
#include <sqlite/sqlite3.h>
#include <config/config.h>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include "databaseAsset.h"

class Database
{

	sqlite3* _db;
	static int sqliteCallback(void *callback, int argc, char **argv, char **azColName);

	std::unordered_map<size_t, std::string> _permissions;
public:
	struct sqlColumn
	{
		char* name;
		char* value;
	};
	typedef std::function<void(const std::vector<sqlColumn>& columns)> sqlCallbackFunction;
	Database();
	~Database();
	void rawSQLCall(const std::string& cmd, const sqlCallbackFunction& f);
	bool stringSafe(const std::string& str);

	std::string getUserID(const std::string& username);
	std::unordered_set<std::string> userPermissions(const std::string& userID);

	AssetData loadAssetData(const AssetID& id);
	AssetPermission assetPermission(const uint32_t assetID, const uint32_t userID);
	std::string assetName(AssetID& id);

	std::vector<AssetData> listUserAssets(const uint32_t& userID);
};


#endif //BRANEENGINE_DATABASE_H
