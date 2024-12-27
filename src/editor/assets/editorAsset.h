//
// Created by eli on 8/18/2022.
//

#ifndef BRANEENGINE_EDITORASSET_H
#define BRANEENGINE_EDITORASSET_H

#include <filesystem>
#include <assets/assetType.h>
#include <utility/jsonVersioner.h>
#include <utility/serializedData.h>

class Asset;

class BraneProject;

class EditorAsset
{
  protected:
    // Editable data goes in the json for easy versioning, other things will be serialized in binary format
    BraneProject& _project;
    VersionedJson _json;
    std::filesystem::path _file;
    AssetType _type;
    std::string _name;

  public:
    static EditorAsset* openUnknownAsset(const std::filesystem::path& path, BraneProject& project);

    EditorAsset(const std::filesystem::path& file, BraneProject& project);

    virtual ~EditorAsset() = default;

    bool load();

    virtual std::vector<std::pair<AssetID, AssetType>> containedAssets() const = 0;

    virtual Asset* buildAsset(const AssetID& id) const = 0;

    std::string hash(const AssetID& id);

    const AssetType& type() const;

    const std::string& name() const;

    const std::filesystem::path& file() const;

    bool unsavedChanges() const;

    void save();

    AssetID id() const;

    VersionedJson& json();
};

#endif // BRANEENGINE_EDITORASSET_H
