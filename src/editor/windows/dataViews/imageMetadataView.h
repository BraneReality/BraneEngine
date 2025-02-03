#pragma once
#include "dataView.h"
#include "editor/assets/sources/imageSource.h"
#include "editor/editor.h"

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
