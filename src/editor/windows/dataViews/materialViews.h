#pragma once
#include "dataView.h"
#include "editor/assets/sources/materialSource.h"
#include "editor/editor.h"

class MaterialSourceView : public DataView
{
    Editor* _editor;
    Shared<MaterialAssetSource> _source;

  public:
    MaterialSourceView(Shared<MaterialAssetSource> source);

    Result<void> draw() override;
};
