//
// Created by eli on 5/25/2022.
//

#ifndef BRANEENGINE_CAMERA_H
#define BRANEENGINE_CAMERA_H

#include "glm/gtx/quaternion.hpp"
#include "glm/mat4x4.hpp"

#include "ecs/nativeComponent.h"

namespace graphics
{

    class Camera : public NativeComponent<Camera>
    {
      public:
        REGISTER_MEMBERS_1("camera", fov, "fov");
        float fov = 45;

        glm::mat4 perspectiveMatrix(glm::uvec2 targetSize) const;
    };

} // namespace graphics

#endif // BRANEENGINE_CAMERA_H
