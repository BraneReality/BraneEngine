//
// Created by eli on 3/11/2022.
//

#ifndef BRANEENGINE_SCRIPTASSET_H
#define BRANEENGINE_SCRIPTASSET_H

#include <vector>
#include "../asset.h"

class ScriptAsset : public Asset
{
  public:
    std::string scriptText;

    ScriptAsset();

    void serialize(OutputSerializer& s) const override;

    void deserialize(InputSerializer& s) override;
};
#endif // BRANEENGINE_SCRIPTASSET_H
