//
// Created by eli on 3/11/2022.
//

#include "scriptAsset.h"

#include "utility/serializedData.h"

ScriptAsset::ScriptAsset() { type.set(AssetType::script); }

void ScriptAsset::serialize(OutputSerializer& s) const
{
    Asset::serialize(s);
    s << scriptText;
}

void ScriptAsset::deserialize(InputSerializer& s)
{
    Asset::deserialize(s);
    s >> scriptText;
}
