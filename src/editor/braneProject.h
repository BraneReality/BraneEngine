//
// Created by eli on 8/14/2022.
//

#ifndef BRANEENGINE_BRANEPROJECT_H
#define BRANEENGINE_BRANEPROJECT_H

#include <filesystem>
#include <string>
#include "assets/assetID.h"
#include "assets/assetType.h"
#include "utility/jsonVersioner.h"
#include <json/json.h>
#include <unordered_map>

class FileWatcher;

class EditorAsset;

class Editor;

// This file stores everything to do with our connection to the server
class BraneProject
{
    Editor& _editor;
    bool _loaded = false;
    std::filesystem::path _filepath;
    VersionedJson _file;
    std::unique_ptr<FileWatcher> _fileWatcher;
    std::unordered_map<std::string, std::shared_ptr<EditorAsset>> _openAssets;

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

    VersionedJson& json();

    Editor& editor();

    FileWatcher* fileWatcher();
};

#endif // BRANEENGINE_BRANEPROJECT_H
