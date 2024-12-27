//
// Created by eli on 5/25/2022.
//

#include "camera.h"

namespace graphics
{

    glm::mat4 Camera::perspectiveMatrix(glm::uvec2 targetSize) const
    {
        glm::mat4 projection = glm::scale(
            glm::perspectiveRH_NO(
                glm::radians(fov), static_cast<float>(targetSize.x) / static_cast<float>(targetSize.y), 0.1f, 100.0f),
            {1.0f, -1.0f, -1.0f});

        return projection;
    }
} // namespace graphics