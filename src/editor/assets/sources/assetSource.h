#pragma once
#include <filesystem>
#include <memory>
#include "assets/assetID.h"
#include "editor/state/trackedObject.h"
#include "utility/option.h"
#include "utility/sptr.h"
#include <assets/assetType.h>

class Asset;

struct AssetSourceID
{
    Json::Value data;
    bool operator==(const AssetSourceID& o) const;
    bool operator!=(const AssetSourceID& o) const;
};

template<>
class std::hash<AssetSourceID>
{
  public:
    size_t operator()(const AssetSourceID& id) const
    {
        return std::hash<std::string>()(id.data.toStyledString());
    }
};

class EditorAsset;

struct AssetMetadata;
struct ImageAssetSource;
struct ImageAssetMetadata;
using AssetSourceType = std::variant<std::shared_ptr<ImageAssetSource>>;
using AssetMetadataType = std::variant<std::shared_ptr<ImageAssetMetadata>>;

class SaveableObject : public TrackedObject
{
  protected:
    int changeDelta = 0;
    bool newChangePath = false;

  public:
    bool unsavedChanges() const;
    void setSaved();

    void onChildForward(EditorActionType at) override;
    void onChildBack(EditorActionType at) override;
    void onNewChange() override;
};

struct AssetSource : public SaveableObject
{
    std::filesystem::path path;

    AssetSource(std::filesystem::path path);
    virtual ~AssetSource() = default;
    virtual std::vector<std::pair<AssetSourceID, AssetType>> exportedAssets() const = 0;
    virtual Result<std::shared_ptr<AssetMetadata>> buildMetadata(const AssetSourceID& id) const = 0;
    virtual Result<std::shared_ptr<Asset>> buildAsset(std::shared_ptr<AssetMetadata> metadata) const = 0;
    virtual Result<void> save() = 0;
    virtual AssetSourceType type() = 0;

    static bool isAssetSource(const std::filesystem::path& path);
    static Result<std::shared_ptr<AssetSource>> load(const std::filesystem::path& path);
};

struct AssetMetadata : public SaveableObject
{
    AssetSourceID sourceId;
    SPtr<TrackedValue<AssetID>> exportId;

    AssetMetadata(AssetSourceID sourceId, AssetID exportId);
    virtual ~AssetMetadata() = default;

    void initMembers(Option<std::shared_ptr<TrackedType>> parent) override;

    virtual Result<Json::Value> serialize() const = 0;
    static Result<std::shared_ptr<AssetMetadata>> deserialize(const Json::Value& root);

    virtual std::string typeName() const = 0;
    virtual AssetMetadataType type() = 0;
    virtual AssetType assetType() const = 0;
};

template<>
struct JsonSerializer<AssetMetadata>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, AssetMetadata& value)
    {
        value.sourceId.data = s["sourceId"];
        CHECK_RESULT(JsonParseUtil::read(s["exportId"], value.exportId));
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const AssetMetadata& value)
    {
        s["sourceId"] = value.sourceId.data;
        CHECK_RESULT(JsonParseUtil::write(s["exportId"], value.exportId));
        return Ok<void>();
    }
};
