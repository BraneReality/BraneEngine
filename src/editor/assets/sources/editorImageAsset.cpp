//
// Created by eli on 11/30/2022.
//

#include "editorImageAsset.h"
#include "assets/types/imageAsset.h"
#include "runtime/runtime.h"

#include <stb_image.h>

EditorImageAsset::EditorImageAsset(const std::filesystem::path& file, BraneProject& project)
    : EditorAsset(file, project)
{
    // Generate default
    if(!std::filesystem::exists(_file))
    {
        _data.data()["imageType"] = 0;
    }
}

std::vector<std::pair<AssetID, AssetType>> EditorImageAsset::containedAssets() const
{
    std::vector<std::pair<AssetID, AssetType>> deps;
    auto res = AssetID::parse(_data["id"].asString());
    if(!res)
    {
        Runtime::warn(std::format("{} has an invalid id", _file.string()));
        return deps;
    }
    deps.emplace_back(res.ok(), AssetType::image);
    return std::move(deps);
}

Asset* EditorImageAsset::buildAsset(const AssetID& id) const
{
    assert(id.toString() == _data["id"].asString());
    auto* asset = new ImageAsset();

    asset->name = name();
    asset->id = id;
    asset->imageType = (ImageAsset::ImageType)_data["imageType"].asUInt();
    std::filesystem::path imagePath = _file.parent_path() / _data["source"].asString();

    int w, h, texChannels;
    unsigned char* pixels = stbi_load(imagePath.string().c_str(), &w, &h, &texChannels, STBI_rgb_alpha);
    if(!pixels)
    {
        Runtime::error("Unable to load image");
        return nullptr;
    }

    asset->size = {w, h};
    asset->data.resize(w * h * 4);
    std::memcpy(asset->data.data(), pixels, asset->data.size());

    stbi_image_free(pixels);

    return asset;
}

void EditorImageAsset::updateSource(const std::filesystem::path& source)
{
    _data.data()["source"] = std::filesystem::relative(source, _file.parent_path()).string();
    save();
}
