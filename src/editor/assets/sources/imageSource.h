#pragma once
#include "assets/types/imageAsset.h"
#include "assetSource.h"
#include "utility/shared.h"

struct ImageAssetSource : public AssetSource
{
    ImageAssetSource(std::filesystem::path path);
    std::vector<std::pair<AssetSourceID, AssetType>> exportedAssets() const override;
    Result<std::shared_ptr<AssetMetadata>> buildMetadata(const AssetSourceID& id) const override;
    Result<std::shared_ptr<Asset>> buildAsset(std::shared_ptr<AssetMetadata> metadata) const override;
    Result<void> save() override;
    AssetSourceType type() override;
};

struct ImageAssetMetadata : public AssetMetadata
{
    Shared<TrackedValue<ImageAsset::ImageType>> colorSpace;
    void initMembers(Option<std::shared_ptr<TrackedType>> parent) override;
    std::string typeName() const override;
    Result<Json::Value> serialize() const override;
    AssetType assetType() const override;
    AssetMetadataType type() override;

    ImageAssetMetadata();
};

template<>
struct JsonSerializer<ImageAssetMetadata>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, ImageAssetMetadata& value)
    {
        CHECK_RESULT(JsonParseUtil::read<AssetMetadata>(s, value));
        CHECK_RESULT(JsonParseUtil::read(s["colorSpace"], value.colorSpace));
        value.setSaved();
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const ImageAssetMetadata& value)
    {
        CHECK_RESULT(JsonParseUtil::write<AssetMetadata>(s, value));
        CHECK_RESULT(JsonParseUtil::write(s["colorSpace"], value.colorSpace));
        return Ok<void>();
    }
};

template<>
struct JsonSerializer<ImageAsset::ImageType>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, ImageAsset::ImageType& value)
    {
        std::string text;
        CHECK_RESULT(JsonParseUtil::read(s, text));
        if(text == "color")
            value = ImageAsset::color;
        else if(text == "normal")
            value = ImageAsset::normal;
        else if(text == "linear")
            value = ImageAsset::linear;
        else
            return Err(JsonSerializerError(JsonSerializerError::WrongStringFormat,
                                           std::format("Expected ImageType enum value, found {}", text)));

        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const ImageAsset::ImageType& value)
    {

        std::string text;
        switch(value)
        {
            case ImageAsset::color:
                text = "color";
                break;
            case ImageAsset::normal:
                text = "normal";
                break;
            case ImageAsset::linear:
                text = "linear";
                break;
            default:
                return Err(JsonSerializerError(JsonSerializerError::ParserError,
                                               "color type serializer not fullly implemented"));
        }
        s = text;

        return Ok<void>();
    }
};
