//
// Created by eli on 11/30/2022.
//

#ifndef BRANEENGINE_IMAGEASSET_H
#define BRANEENGINE_IMAGEASSET_H

#include "../asset.h"
#include "glm/vec2.hpp"

class ImageAsset : public Asset
{
  public:
    std::vector<uint8_t> data;
    glm::uvec2 size;

    enum ImageType : uint8_t
    {
        color = 0,
        normal = 1,
        linear = 2
    } imageType;

    ImageAsset();

#ifdef CLIENT
    uint32_t runtimeID = -1;
    bool imageUpdated = false;
    void onDependenciesLoaded() override;
#endif

    void serialize(OutputSerializer& s) const override;

    void deserialize(InputSerializer& s) override;

    void serializeHeader(OutputSerializer& s) const;

    void deserializeHeader(InputSerializer& s);

    // std::unique_ptr<SerializationContext> createContext() const;

    // bool serializeIncrement(OutputSerializer& sData, SerializationContext* iteratorData) const;

    void deserializeIncrement(InputSerializer& sData);
};

#endif // BRANEENGINE_IMAGEASSET_H
