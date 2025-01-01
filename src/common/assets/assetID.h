#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include "utility/option.h"
#include "utility/serializedData.h"
#include "utility/uuid.h"
#include <string_view>

struct BraneAssetID
{
    std::string domain;
    UUID uuid;

    static Result<BraneAssetID> parse(std::string_view text);

    std::string toString() const;
};

struct FileAssetID
{
    std::string path;
    static Result<FileAssetID> parse(std::string_view text);

    std::string toString() const;
};

struct AssetID
{
    typedef std::variant<BraneAssetID, FileAssetID> Protocol;
    Option<Protocol> value;

    AssetID() = default;

    inline AssetID(BraneAssetID id) : value(Some<Protocol>(std::move(id))) {};
    inline AssetID(FileAssetID id) : value(Some<Protocol>(std::move(id))) {};

    static Result<AssetID> parse(std::string_view text);

    bool empty() const;

    void clear();

    std::string toString() const;

    bool operator==(const AssetID&) const;
    bool operator!=(const AssetID&) const;

    friend std::ostream& operator<<(std::ostream& os, const AssetID& id);

    template<class T>
    Option<T&> as()
    {
        if(!value)
            return None();
        if(std::holds_alternative<T>(value.value()))
            return Some<T&>(std::get<T>(value.value()));
        return None();
    }

    template<class T>
    Option<const T&> as() const
    {
        if(!value)
            return None();
        if(std::holds_alternative<T>(value.value()))
            return Some<const T&>(std::get<T>(value.value()));
        return None();
    }
};

template<>
struct std::hash<BraneAssetID>
{
    std::size_t operator()(const BraneAssetID& id) const
    {
        return std::hash<UUID>()(id.uuid);
    }
};

template<>
struct std::hash<FileAssetID>
{
    std::size_t operator()(const FileAssetID& id) const
    {
        return std::hash<std::string>()(id.path);
    }
};

template<>
struct std::hash<AssetID>
{
    std::size_t operator()(const AssetID& id) const
    {
        if(!id.value)
        {
            return 0;
        }
        return MATCHV(id.value.value(), [](const BraneAssetID& id) {
            return std::hash<BraneAssetID>()(id);
        }, [](const FileAssetID& id) { return std::hash<FileAssetID>()(id); });
    }
};

template<>
struct Serializer<BraneAssetID>
{
    static Result<void, SerializerError> read(InputSerializer& s, BraneAssetID& value)
    {
        auto res = s >> value.domain >> value.uuid;
        if(!res)
            return Err(res.err());
        return Ok();
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const BraneAssetID& value)
    {

        auto res = s << value.domain << value.uuid;
        if(!res)
            return Err(res.err());
        return Ok();
    }
};

template<>
struct Serializer<FileAssetID>
{
    static Result<void, SerializerError> read(InputSerializer& s, FileAssetID& value)
    {
        auto res = s >> value.path;
        if(!res)
            return Err(res.err());
        return Ok();
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const FileAssetID& value)
    {

        auto res = s << value.path;
        if(!res)
            return Err(res.err());
        return Ok();
    }
};

template<>
struct Serializer<AssetID>
{
    static Result<void, SerializerError> read(InputSerializer& s, AssetID& value)
    {
        auto res = s >> value.value;
        if(!res)
            return Err(res.err());
        return Ok();
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const AssetID& value)
    {
        auto res = s << value.value;
        if(!res)
            return Err(res.err());
        return Ok();
    }
};
