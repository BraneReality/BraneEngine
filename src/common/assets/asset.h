#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include "assets/assetID.h"
#include <assets/assetType.h>

class AssetManager;

class InputSerializer;

class OutputSerializer;

struct AssetDependency
{
    const AssetID& id;
    bool streamable = false;
};

class Asset
{
    static Asset* assetFromType(AssetType type);

  public:
    std::string name;
    AssetID id;
    AssetType type;

    Asset() = default;

    Asset(Asset&&) = default;

    Asset& operator=(Asset&&) = default;

    virtual ~Asset() = default;

    static Asset* deserializeUnknown(InputSerializer& s);

    virtual void serialize(OutputSerializer& s) const;

    virtual void deserialize(InputSerializer& s);

    virtual std::vector<AssetDependency> dependencies() const;

    virtual void onDependenciesLoaded();
};

class IncrementalAsset : public Asset
{
  public:
    struct SerializationContext
    {
    };

    static IncrementalAsset* deserializeUnknownHeader(InputSerializer& s);

    virtual void serializeHeader(OutputSerializer& s) const;

    virtual void deserializeHeader(InputSerializer& s);

    virtual bool serializeIncrement(OutputSerializer& s, SerializationContext* iteratorData) const;

    virtual void deserializeIncrement(InputSerializer& s) = 0;

    virtual std::unique_ptr<SerializationContext> createContext() const = 0;

    virtual void onFullyLoaded();
};
