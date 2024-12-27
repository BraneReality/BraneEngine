#include "assetID.h"
#include <cassert>
#include <sstream>
#include <utility/hex.h>

AssetID::AssetID(const std::string& serverAddress, uint32_t id)
{
    _string = serverAddress + "/" + toHex(id);
    _delimiter = serverAddress.size();
    _idCache = id;
}

AssetID::AssetID(std::string&& id)
{
    if(id == "")
        id = "null";
    assert(id == "null" || id.find('/') != std::string::npos);
    _string = std::move(id);
    _delimiter = _string.find('/');
    _idCache =
        fromHex<uint32_t>(std::string_view{_string.data() + (_delimiter + 1), _string.size() - (_delimiter + 1)});
}

AssetID::AssetID(const std::string& id)
{
    assert(id == "null" || id.find('/') != std::string::npos);
    _string = id;
    _delimiter = _string.find('/');
    _idCache =
        fromHex<uint32_t>(std::string_view{_string.data() + (_delimiter + 1), _string.size() - (_delimiter + 1)});
}

const std::string& AssetID::string() const { return _string; }

bool AssetID::operator==(const AssetID& other) const { return _string == other._string; }

bool AssetID::operator!=(const AssetID& other) const { return _string != other._string; }

std::ostream& operator<<(std::ostream& os, const AssetID& id)
{
    os << id.string();
    return os;
}

bool AssetID::null() const { return _delimiter == std::string::npos; }

AssetID& AssetID::operator=(std::string&& id)
{
    assert(id == "null" || id.find('/') != std::string::npos);
    _string = std::move(id);
    _delimiter = _string.find('/');
    _idCache = fromHex<uint32_t>({_string.data() + (_delimiter + 1), _string.size() - (_delimiter + 1)});
    return *this;
}

AssetID& AssetID::operator=(const std::string& id)
{
    assert(id == "null" || id.find('/') != std::string::npos);
    _string = id;
    _delimiter = _string.find('/');
    _idCache = fromHex<uint32_t>({_string.data() + (_delimiter + 1), _string.size() - (_delimiter + 1)});
    return *this;
}

uint32_t AssetID::id() const
{
    assert(!null());
    return _idCache;
}

std::string_view AssetID::address() const
{
    assert(!null());
    return {_string.data(), _delimiter};
}

void AssetID::setID(uint32_t newID)
{
    if(null())
    {
        _string = "/";
        _delimiter = 0;
    }
    _string = _string.replace(
        _string.begin() + static_cast<std::string::difference_type>(_delimiter + 1), _string.end(), toHex(newID));
}

void AssetID::setAddress(std::string_view newAddress)
{
    if(null())
    {
        _string = "/";
        _delimiter = 0;
    }
    _string = _string.replace(
        _string.begin(), _string.begin() + static_cast<std::string::difference_type>(_delimiter), newAddress);
    _delimiter = newAddress.size();
}

void AssetID::setNull()
{
    _string = "null";
    _delimiter = std::string::npos;
}

std::string_view AssetID::idStr() const
{
    assert(!null());
    return {_string.data() + _delimiter + 1, _string.size() - (_delimiter + 1)};
}

AssetID& AssetID::operator=(AssetID&& o) noexcept
{
    _string = std::move(o._string);
    _delimiter = o._delimiter;
    _idCache = o._idCache;
    o._string = "null";
    o._delimiter = std::string::npos;
    return *this;
}

AssetID::AssetID(AssetID&& o)
{
    _string = std::move(o._string);
    _delimiter = o._delimiter;
    _idCache = o._idCache;
    o._string = "null";
    o._delimiter = std::string::npos;
}

AssetID AssetID::sameOrigin(const AssetID& parent)
{
    AssetID id = *this;
    id.setAddress(parent.address());
    return id;
}

std::size_t std::hash<HashedAssetID>::operator()(const HashedAssetID& k) const { return k.hash(); }

HashedAssetID::HashedAssetID(const AssetID& id)
{
    _hash = std::hash<std::string>()(id.string());
    _id = id.id();
    _address = id.address();
}

bool HashedAssetID::operator!=(const HashedAssetID& hash) const { return !(*this == hash); }

bool HashedAssetID::operator==(const HashedAssetID& hash) const
{
    return _hash == hash._hash && _id == hash._id && _address == hash._address;
}

size_t HashedAssetID::hash() const { return _hash; }
