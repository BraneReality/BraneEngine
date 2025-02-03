//
// Created by eli on 8/14/2022.
//

#ifndef BRANEENGINE_BRANEPROJECT_H
#define BRANEENGINE_BRANEPROJECT_H

#include <filesystem>
#include <string>
#include "assets/assetID.h"
#include "editor/assets/assetIndexer.h"
#include "editor/assets/sources/assetSource.h"
#include "editor/state/trackedObject.h"
#include "utility/shared.h"
#include <json/json.h>
#include <unordered_map>

class FileWatcher;

class EditorAsset;

class Editor;

struct BraneProjectData;

struct BraneAssetServerInfo : public TrackedObject
{
    Shared<TrackedValue<std::string>> address;
    Shared<TrackedValue<uint32_t>> port;
    BraneAssetServerInfo();
    void initMembers(Option<std::shared_ptr<TrackedType>> parent) override;
};

struct BraneProjectData : public SaveableObject
{
    Shared<TrackedValue<std::string>> name;
    Shared<BraneAssetServerInfo> assetServer;
    BraneProjectData();
    void initMembers(Option<std::shared_ptr<TrackedType>> parent) override;
};

// This file stores everything to do with our connection to the server
class BraneProject
{
    std::filesystem::path _root;
    Shared<BraneProjectData> _data;
    AssetIndexer _indexer;
    RwMutex<std::unordered_map<AssetSourceID, std::shared_ptr<EditorAsset>>> _openAssets;

    void initLoaded();

  public:
    BraneProject(std::filesystem::path root);
    BraneProject(BraneProject&&) = default;
    BraneProject& operator=(BraneProject&&) = default;

    ~BraneProject();

    static Result<BraneProject> load(const std::filesystem::path& filepath, bool initProject = true);

    static Result<BraneProject> create(const std::string& projectName, const std::filesystem::path& directory);

    void save();

    bool unsavedChanges() const;

    std::filesystem::path root();

    Option<std::shared_ptr<EditorAsset>> getEditorAsset(const AssetID& id);
    Option<std::shared_ptr<EditorAsset>> getEditorAsset(const std::filesystem::path& path);

    BraneProjectData& data();

    AssetIndexer& indexer();
};

#endif // BRANEENGINE_BRANEPROJECT_H
