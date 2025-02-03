#ifndef BRANEENGINE_EDITORASSET_H
#define BRANEENGINE_EDITORASSET_H

#include "assets/asset.h"
#include "editor/assets/sources/imageSource.h"
#include "utility/shared.h"
#include <utility/serializedData.h>

class BraneProject;

/// An editor asset is a source-metadata pair that can be baked into one or more runtime assets
class EditorAsset : public TrackedObject
{
  protected:
    Shared<AssetSource> _source;
    std::unordered_map<AssetSourceID, Shared<AssetMetadata>> _metadata;
    std::unordered_map<AssetID, AssetSourceID> _exportedToSource;

  public:
    EditorAsset(const std::filesystem::path& sourcePath, std::shared_ptr<AssetSource> source);
    static Result<std::shared_ptr<EditorAsset>> loadAsset(const std::filesystem::path& sourcePath);
    static bool isAssetSource(const std::filesystem::path& sourcePath);

    void initMembers();

    ~EditorAsset() = default;

    Shared<AssetSource> source() const;
    const std::unordered_map<AssetSourceID, Shared<AssetMetadata>>& metadata() const;
    Option<Shared<AssetMetadata>> getMetadata(AssetSourceID id) const;
    Option<Shared<AssetMetadata>> getMetadata(AssetID id) const;

    std::vector<std::pair<AssetID, AssetType>> exportedAssets() const;
    Result<std::shared_ptr<Asset>> buildAsset(const AssetID& id) const;
    Result<std::shared_ptr<Asset>> buildAsset(const AssetSourceID& id) const;

    std::string hash() const;
    std::string name() const;

    const std::filesystem::path& sourcePath() const;
    std::filesystem::path metadataPath() const;

    bool unsavedChanges() const;
    void save();
};

#endif // BRANEENGINE_EDITORASSET_H
