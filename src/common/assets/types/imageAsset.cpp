//
// Created by eli on 11/30/2022.
//

#include "imageAsset.h"
#include "utility/serializedData.h"

#ifdef CLIENT
#include "graphics/graphics.h"
#include "runtime/runtime.h"
#endif

struct ImageSerializationContext : IncrementalAsset::SerializationContext
{
    uint32_t pos = 0;
};

ImageAsset::ImageAsset() { type.set(AssetType::Type::image); }

#ifdef CLIENT
void ImageAsset::onDependenciesLoaded()
{
    Asset::onDependenciesLoaded();
    runtimeID = Runtime::getModule<graphics::VulkanRuntime>()->addAsset(this);
}
#endif

void ImageAsset::serialize(OutputSerializer& s) const
{
    Asset::serialize(s);
    s << imageType << size << data;
}

void ImageAsset::deserialize(InputSerializer& s)
{
    Asset::deserialize(s);
    s >> imageType >> size >> data;
}

void ImageAsset::serializeHeader(OutputSerializer& s) const
{
    IncrementalAsset::serializeHeader(s);
    s << imageType << size << (uint32_t)data.size();
}

void ImageAsset::deserializeHeader(InputSerializer& s)
{
    IncrementalAsset::deserializeHeader(s);
    uint32_t dataSize;
    s >> imageType >> size >> dataSize;
    data.resize(dataSize);

    // Initialize color with a grid;
    uint32_t lineWidth = 2;
    uint32_t lineSpacing = 10;
    uint8_t gridColor[4] = {50, 168, 82, 255};
    for(uint32_t y = 0; y < size.y; ++y)
    {
        bool yLine = y % lineSpacing < lineWidth;
        for(uint32_t x = 0; x < size.x; ++x)
        {
            uint32_t pixel = y * size.x + x;
            if(yLine || x % lineSpacing < lineWidth)
                ((uint32_t*)data.data())[pixel] = *((uint32_t*)gridColor);
            else
                ((uint32_t*)data.data())[pixel] = 255;
        }
    }
}

std::unique_ptr<IncrementalAsset::SerializationContext> ImageAsset::createContext() const
{
    return std::unique_ptr<ImageSerializationContext>();
}

bool ImageAsset::serializeIncrement(OutputSerializer& s, IncrementalAsset::SerializationContext* iteratorData) const
{
    auto* ctx = (ImageSerializationContext*)iteratorData;
    const uint32_t squareSize = 50;

    s << ctx->pos << squareSize;

    glm::uvec2 start = {ctx->pos % size.x, ctx->pos / size.x};
    glm::uvec2 end = {std::min(start.x + squareSize, size.x), std::min(start.y + squareSize, size.y)};
    for(uint32_t y = start.y; y < end.y; ++y)
    {
        for(uint32_t x = start.x; x < end.x; ++x)
        {
            uint32_t pixel = y * size.x + x;
            s << ((uint32_t*)data.data())[pixel];
            ctx->pos++;
        }
    }

    return ctx->pos != size.x * size.y;
}

void ImageAsset::deserializeIncrement(InputSerializer& s)
{
    uint32_t pos, squareSize;
    s >> pos >> squareSize;

    glm::uvec2 start = {pos % size.x, pos / size.x};
    glm::uvec2 end = {std::min(start.x + squareSize, size.x), std::min(start.y + squareSize, size.y)};
    for(uint32_t y = start.y; y < end.y; ++y)
    {
        for(uint32_t x = start.x; x < end.x; ++x)
        {
            uint32_t pixel = y * size.x + x;
            s >> ((uint32_t*)data.data())[pixel];
        }
    }
}
