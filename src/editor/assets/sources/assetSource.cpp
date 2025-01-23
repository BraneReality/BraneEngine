#include "assetSource.h"
#include <memory>
#include "../editorAsset.h"
#include <unordered_set>

static std::unordered_set<std::string> assetSourceExtensions = {
    ".png", ".gltf", ".glb", ".b-assembly", ".b-material", ".b-chunk", ".vert", ".frag"};

static bool isAssetSource(const std::filesystem::path& path)
{
    auto ext = path.extension().string();
    return assetSourceExtensions.contains(ext);
}

static Result<std::shared_ptr<AssetSource>> load(const std::filesystem::path& path)
{
    return Err((std::string) "Not implemented");
}

bool AssetSourceID::operator==(const AssetSourceID& o) const
{
    return data == o.data;
}

bool AssetSourceID::operator!=(const AssetSourceID& o) const
{
    return data != o.data;
}

bool SaveableObject::unsavedChanges() const
{
    return changeDelta == 0 && !newChangePath;
}

void SaveableObject::setSaved()
{
    changeDelta = 0;
    newChangePath = false;
}

void SaveableObject::onChildForward(EditorActionType at)
{
    changeDelta++;
    TrackedObject::onChildForward(at);
}

void SaveableObject::onChildBack(EditorActionType at)
{
    changeDelta--;
    TrackedObject::onChildBack(at);
}

void SaveableObject::onNewChange()
{
    // This means we can never undo back to the state we saved, because we just overwrote that history
    if(changeDelta < 0)
        newChangePath = true;
    TrackedObject::onNewChange();
}

AssetSource::AssetSource(std::filesystem::path path) : path(path) {}

AssetMetadata::AssetMetadata(AssetSourceID sourceId, AssetID exportId)
    : sourceId(std::move(sourceId)), exportId(std::move(exportId))
{}

void AssetMetadata::initMembers(Option<std::shared_ptr<TrackedType>> parent)
{
    TrackedType::initMembers(parent);
    auto p = Some(shared_from_this());
    exportId->initMembers(p);
}

Result<std::shared_ptr<AssetMetadata>> AssetMetadata::deserialize(const Json::Value& root)
{
    auto metadataType = root.get("metadataType", "none").asString();
    if(metadataType == "none")
        return Err(std::string("metadataType key was missing from metadata"));

    if(metadataType == "Image")
    {
        auto md = std::make_shared<ImageAssetMetadata>();
        auto res = JsonParseUtil::read(root, *md);
        if(!res)
            return Err(res.err().toString());
        return Ok<std::shared_ptr<AssetMetadata>>(md);
    }

    return Err(std::format("Metadata type of '{}' unrecongized", metadataType));
}
