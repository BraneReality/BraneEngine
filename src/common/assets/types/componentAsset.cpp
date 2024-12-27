#include "componentAsset.h"
#include "common/ecs/component.h"
#include "utility/serializedData.h"

ComponentAsset::ComponentAsset() { type.set(AssetType::Type::component); }

ComponentAsset::ComponentAsset(const std::vector<VirtualType::Type>& members,
                               const std::vector<std::string>& memberNames,
                               AssetID&& id)
{
    this->id = std::move(id);
    type.set(AssetType::Type::component);
    _members = members;
    _memberNames = memberNames;
}

ComponentAsset::~ComponentAsset() {}

const std::vector<VirtualType::Type>& ComponentAsset::members() const { return _members; }

void ComponentAsset::serialize(OutputSerializer& s) const
{
    Asset::serialize(s);
    s << _members << _memberNames;
}

void ComponentAsset::deserialize(InputSerializer& s)
{
    Asset::deserialize(s);
    s >> _members >> _memberNames;
}

const std::vector<std::string>& ComponentAsset::memberNames() const { return _memberNames; }

std::vector<VirtualType::Type>& ComponentAsset::members() { return _members; }

std::vector<std::string>& ComponentAsset::memberNames() { return _memberNames; }
