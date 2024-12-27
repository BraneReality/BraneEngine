//
// Created by eli on 9/7/2022.
//

#ifndef BRANEENGINE_POINTLIGHTCOMPONENT_H
#define BRANEENGINE_POINTLIGHTCOMPONENT_H

#include "ecs/nativeComponent.h"

class PointLightComponent : public NativeComponent<PointLightComponent>
{
    REGISTER_MEMBERS_1("Point Light", color, "color");

  public:
    glm::vec4 color = {1, 1, 1, 1};
};

#endif // BRANEENGINE_POINTLIGHTCOMPONENT_H
