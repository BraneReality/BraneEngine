#pragma once
#include "dataView.h"
#include "editor/assets/sources/shaderSource.h"
#include "editor/editor.h"

class ShaderSourceView : public DataView
{
    Editor* _editor;
    Shared<ShaderAssetSource> _source;

    Option<Shared<ShaderAsset>> _previewAsset;

  public:
    ShaderSourceView(Shared<ShaderAssetSource> source);

    Result<void> draw() override;
};
