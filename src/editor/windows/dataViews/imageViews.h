#pragma once
#include "dataView.h"
#include "editor/assets/sources/imageSource.h"
#include "editor/editor.h"

/*class ImageSourceView : public DataView
{
    Editor* _editor;
    Shared<ImageAssetSource> _source;

    glm::uvec2 _bounds;
    size_t _size;

  public:
    ImageSourceView(Shared<ImageAssetSource> source);

    Result<void> draw() override;
};*/

class ImageMetadataView : public DataView
{
    Editor* _editor;
    Shared<ImageAssetMetadata> _metadata;

    Option<Shared<ImageAsset>> _previewImageAsset;
    Option<VkDescriptorSet> _imagePreview;

  public:
    ImageMetadataView(Shared<ImageAssetMetadata> metadata);

    Result<void> draw() override;
};
