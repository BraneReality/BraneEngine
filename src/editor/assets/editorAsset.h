#ifndef BRANEENGINE_EDITORASSET_H
#define BRANEENGINE_EDITORASSET_H

#include <filesystem>
#include <memory>
#include "assets/asset.h"
#include "assets/assetID.h"
#include "editor/state/trackedObject.h"
#include <assets/assetType.h>
#include <utility/jsonVersioner.h>
#include <utility/serializedData.h>

class Asset;

class BraneProject;

struct AssetSourceID
{
  public:
    virtual bool operator==(std::shared_ptr<AssetSourceID>) const = 0;
    virtual bool operator!=(std::shared_ptr<AssetSourceID>) const = 0;
    virtual size_t hash() const = 0;
};

template<>
struct std::hash<std::shared_ptr<AssetSourceID>>
{
    size_t operator()(const std::shared_ptr<AssetSourceID>& id)
    {
        return id->hash();
    }
};

struct AssetSource : public TrackedObject
{
    virtual std::vector<std::pair<std::shared_ptr<AssetSourceID>, AssetType>> containedAssets() const = 0;
    virtual Result<std::shared_ptr<Asset>> buildAsset(const AssetID& id) = 0;
};

struct AssetMetadata : public TrackedObject
{
    Tracked<TrackedValue<AssetID>> id;

    virtual std::shared_ptr<AssetSourceID> describedSource() const = 0;
};

/// An editor asset is a source-metadata pair that can be baked into one or more runtime assets
class EditorAsset : public TrackedObject
{
  protected:
    // Filepath is the path to the source asset
    std::filesystem::path _sourcePath;

    std::variant<Tracked<AssetSource>> _source;
    std::map<AssetID, std::variant<Tracked<AssetMetadata>>> _metadata;

    int changeDelta = 0;
    int changeVersion = 0;
    int saveVersion = 0;

  public:
    static Result<std::unique_ptr<EditorAsset>> openUnknownAsset(const std::filesystem::path& sourcePath);

    EditorAsset(const std::filesystem::path& sourcePath);

    virtual ~EditorAsset() = default;

    virtual std::vector<std::pair<AssetID, AssetType>> containedAssets() const = 0;

    virtual Result<std::unique_ptr<Asset>> buildAsset(const AssetID& id) const = 0;

    std::string hash() const;

    const std::string& name() const;

    const std::filesystem::path& dataPath() const;
    const std::filesystem::path metadataPath() const;

    bool unsavedChanges() const;

    void save();
};

#endif // BRANEENGINE_EDITORASSET_H
