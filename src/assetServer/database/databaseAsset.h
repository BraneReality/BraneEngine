//
// Created by eli on 2/5/2022.
//

#ifndef BRANEENGINE_DATABASEASSET_H
#define BRANEENGINE_DATABASEASSET_H

#include <string>
#include "assets/assetType.h"
#include "utility/uuid.h"

class AssetType;

class Database;

enum class AssetPermissionLevel
{
    none = 0,
    view = 1,
    edit = 2,
    owner = 3
};

struct AssetInfo
{
    UUID id;
    std::string name;
    AssetType type;
    std::string hash;
};

#endif // BRANEENGINE_DATABASEASSET_H
