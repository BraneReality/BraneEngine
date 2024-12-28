#include "assetType.h"
#include "utility/enumNameMap.h"

const EnumNameMap<AssetType::Type> names({
    {AssetType::none, "none"},
    {AssetType::component, "component"},
    {AssetType::script, "script"},
    {AssetType::mesh, "mesh"},
    {AssetType::material, "material"},
    {AssetType::image, "image"},
    {AssetType::player, "player"},
    {AssetType::shader, "shader"},
    {AssetType::assembly, "assembly"},
    {AssetType::chunk, "chunk"},
});

AssetType::Type AssetType::fromString(const std::string& type) { return names.toEnum(type); }

const std::string& AssetType::toString(Type type) { return names.toString(type); }

AssetType::AssetType() { _type = none; }

void AssetType::set(const std::string& type) { _type = fromString(type); }

void AssetType::set(AssetType::Type type) { _type = type; }

AssetType::Type AssetType::type() const { return _type; }

const std::string& AssetType::toString() const { return toString(_type); }

bool AssetType::operator==(AssetType::Type t) const { return t == _type; }

bool AssetType::operator==(AssetType t) const { return _type == t._type; }

bool AssetType::operator!=(AssetType t) const { return _type != t._type; }

bool AssetType::operator!=(AssetType::Type t) const { return t != _type; }

AssetType::AssetType(AssetType::Type type) { _type = type; }
