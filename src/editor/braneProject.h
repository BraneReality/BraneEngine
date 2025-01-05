//
// Created by eli on 8/14/2022.
//

#ifndef BRANEENGINE_BRANEPROJECT_H
#define BRANEENGINE_BRANEPROJECT_H

#include <filesystem>
#include <string>
#include "assets/assetID.h"
#include "assets/assetType.h"
#include "editor/assetIndexer.h"
#include "editor/state/trackedObject.h"
#include "utility/jsonVersioner.h"
#include <json/json.h>
#include <unordered_map>

class FileWatcher;

class EditorAsset;

class Editor;

struct BraneProjectData : public TrackedObject
{
    Tracked<TrackedValue<std::string>> name;
};

// This file stores everything to do with our connection to the server
class BraneProject

{
    Editor& _editor;
    bool _loaded = false;
    std::filesystem::path _filepath;
    BraneProjectData _data;
    AssetIndexer _indexer;
    RwMutex<std::unordered_map<std::string, std::shared_ptr<EditorAsset>>> _openAssets;

    void loadDefault();

    void initLoaded();

  public:
    BraneProject(Editor& editor);

    ~BraneProject();

    bool loaded() const;

    bool load(const std::filesystem::path& filepath);

    void create(const std::string& projectName, const std::filesystem::path& directory);

    void save();

    bool unsavedChanges() const;

    std::filesystem::path projectDirectory();

    BraneProjectData& data();

    Editor& editor();

    FileWatcher* fileWatcher();
};

#endif // BRANEENGINE_BRANEPROJECT_H
