#pragma once
#include "utility/result.h"

class DataView
{
  public:
    virtual ~DataView() = default;
    virtual Result<void> draw() = 0;
};
