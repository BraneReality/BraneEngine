#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

class AssetID
{
    std::string _string = "null";
    std::string::size_type _delimiter = std::string::npos;
    uint32_t _idCache = -1;

  public:
    AssetID() = default;

    AssetID(const AssetID&) = default;

    AssetID(AssetID&&);

    explicit AssetID(const std::string& id);

    explicit AssetID(std::string&& id);

    AssetID(const std::string& serverAddress, uint32_t id);

    uint32_t id() const;

    std::string_view idStr() const;

    void setID(uint32_t newID);

    std::string_view address() const;

    void setAddress(std::string_view newAddress);

    const std::string& string() const;

    bool null() const;

    void setNull();

    AssetID sameOrigin(const AssetID& parent);

    AssetID& operator=(const AssetID&) = default;

    AssetID& operator=(AssetID&&) noexcept;

    AssetID& operator=(std::string&& id);

    AssetID& operator=(const std::string& id);

    bool operator==(const AssetID& other) const;

    bool operator!=(const AssetID& other) const;

    friend std::ostream& operator<<(std::ostream& os, const AssetID& id);
};

class HashedAssetID
{
    size_t _hash;
    uint32_t _id;
    std::string _address;

  public:
    HashedAssetID(const AssetID& id);

    bool operator!=(const HashedAssetID& hash) const;

    bool operator==(const HashedAssetID& hash) const;

    size_t hash() const;
};

template<>
struct std::hash<HashedAssetID>
{
    size_t operator()(const HashedAssetID& k) const;
};
