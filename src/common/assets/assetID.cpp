#include "assetID.h"
#include <cassert>
#include <sstream>
#include <utility/hex.h>

std::string BraneAssetID::toString() const
{
    return std::format("brane://{}/{}", domain, uuid.toString());
}

Result<BraneAssetID, std::string> BraneAssetID::parse(std::string_view text)
{
    // Find the '/' delimiter to separate domain and UUID
    auto uuid_start = text.find('/', 0);
    if(uuid_start == std::string_view::npos)
    {
        return Err(std::string("Invalid format: Missing UUID delimiter '/'"));
    }

    // Extract domain and UUID parts
    auto domain = std::string(text.substr(0, uuid_start));
    if(domain.empty())
    {
        return Err(std::string("Invalid format: Domain cannot be empty"));
    }

    std::string_view uuid_str = text.substr(uuid_start + 1);
    if(uuid_str.empty())
    {
        return Err(std::string("Invalid format: UUID cannot be empty"));
    }

    // Parse UUID
    Result<UUID> uuid = UUID::fromString(uuid_str);
    if(!uuid)
        return Err(uuid.err());

    // Return the constructed AssetID
    return Ok<BraneAssetID>(BraneAssetID{domain, uuid.ok()});
}

std::string FileAssetID::toString() const
{
    return std::format("file://{}", path);
}

Result<FileAssetID, std::string> FileAssetID::parse(std::string_view text)
{
    if(text.empty())
        return Err(std::string("path cannot be empty"));
    return Ok<FileAssetID>(FileAssetID{std::string(text)});
}

Result<AssetID, std::string> AssetID::parse(std::string_view text)
{
    if(text == "null" || text == "NULL")
        return Ok(AssetID());

    // Find the "://" delimiter to isolate protocol
    auto protocol_end = text.find("://");
    if(protocol_end == std::string_view::npos)
    {
        return Err(std::string("Invalid format: Missing protocol delimiter '://'"));
    }

    std::string_view protocol_str = text.substr(0, protocol_end);
    std::string_view content_str = text.substr(protocol_end + 1);
    if(protocol_str == "brane")
        return BraneAssetID::parse(content_str).map<AssetID>([](BraneAssetID id) { return std::move(id); });
    else if(protocol_str == "file")
        return FileAssetID::parse(content_str).map<AssetID>([](FileAssetID id) { return std::move(id); });
    else
    {
        return Err("Invalid protocol: " + std::string(protocol_str));
    }
}

bool AssetID::empty() const
{
    return value.isNone();
}

void AssetID::clear()
{
    value = None();
}

bool AssetID::operator==(const AssetID& o) const
{
    return o.value == value;
}

bool AssetID::operator!=(const AssetID& o) const
{
    return !(o == *this);
}

std::string AssetID::toString() const
{
    if(!value)
        return "null";

    return MATCHV(value.value(), [](const auto& id) { return id.toString(); });
}

std::ostream& operator<<(std::ostream& os, const AssetID& id)
{
    return os << id.toString();
}
