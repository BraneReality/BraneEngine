//
// Created by eli on 11/30/2022.
//

#ifndef BRANEENGINE_IMAGEASSET_H
#define BRANEENGINE_IMAGEASSET_H

#include "../asset.h"
#include "glm/vec2.hpp"

class ImageAsset : public IncrementalAsset
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

    void serializeHeader(OutputSerializer& s) const override;

    void deserializeHeader(InputSerializer& s) override;

    std::unique_ptr<SerializationContext> createContext() const override;

    bool serializeIncrement(OutputSerializer& sData, SerializationContext* iteratorData) const override;

    void deserializeIncrement(InputSerializer& sData) override;
};

#endif // BRANEENGINE_IMAGEASSET_H
