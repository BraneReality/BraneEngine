
#include "imageSource.h"
#include <stb_image.h>

ImageAssetSource::ImageAssetSource(std::filesystem::path path) : AssetSource(path) {}

std::vector<std::pair<AssetSourceID, AssetType>> ImageAssetSource::exportedAssets() const
{
    return {{AssetSourceID{Json::Value("main")}, AssetType::image}};
}

Result<std::shared_ptr<AssetMetadata>> ImageAssetSource::buildMetadata(const AssetSourceID& id) const
{
    if(id.data != "main")
        return Err(std::string("ImageAsset only exports 'main'"));
    return Ok<std::shared_ptr<AssetMetadata>>(std::make_shared<ImageAssetMetadata>());
}

Result<std::shared_ptr<Asset>> ImageAssetSource::buildAsset(std::shared_ptr<AssetMetadata> metadata) const
{
    auto imd = std::dynamic_pointer_cast<ImageAssetMetadata>(metadata);
    if(!imd)
        return Err((std::string) "Incorrect metadata passed");

    auto image = std::make_shared<ImageAsset>();
    image->name = path.stem().string();
    image->id = *metadata->exportId->value();
    image->imageType = *imd->colorSpace->value();

    int w, h, texChannels;
    unsigned char* pixels = stbi_load(path.string().c_str(), &w, &h, &texChannels, STBI_rgb_alpha);
    if(!pixels)
        return Err(std::format("Failed to load image at {}", path.string()));

    image->size = {w, h};
    image->data.resize(w * h * 4);
    std::memcpy(image->data.data(), pixels, image->data.size());

    stbi_image_free(pixels);

    return Ok<std::shared_ptr<Asset>>(image);
}

Result<void> ImageAssetSource::save()
{
    return Ok<void>();
}

AssetSourceType ImageAssetSource::type()
{
    return std::static_pointer_cast<ImageAssetSource>(shared_from_this());
}

ImageAssetMetadata::ImageAssetMetadata()
    : AssetMetadata({Json::Value("main")}, BraneAssetID{"", UUID::generate().ok()}),
      colorSpace(ImageAsset::ImageType::color)
{}

void ImageAssetMetadata::initMembers(Option<std::shared_ptr<TrackedType>> parent)
{
    AssetMetadata::initMembers(parent);
    auto p = Some(shared_from_this());
    colorSpace->initMembers(p);
}

Result<Json::Value> ImageAssetMetadata::serialize() const
{
    Json::Value value = Json::Value(Json::ValueType::objectValue);
    auto res = JsonParseUtil::write(value, *this);
    if(!res)
        return Err(res.err().toString());
    value["metadataType"] = typeName();
    return Ok<Json::Value>(value);
}

std::string ImageAssetMetadata::typeName() const
{
    return "Image";
}

AssetType ImageAssetMetadata::assetType() const
{
    return AssetType::image;
}

AssetMetadataType ImageAssetMetadata::type()
{
    return std::static_pointer_cast<ImageAssetMetadata>(shared_from_this());
}
