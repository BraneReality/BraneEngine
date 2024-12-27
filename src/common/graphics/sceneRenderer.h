//
// Created by eli on 5/29/2022.
//

#ifndef BRANEENGINE_SCENERENDERER_H
#define BRANEENGINE_SCENERENDERER_H

#include "glm/gtx/quaternion.hpp"
#include "graphicsBuffer.h"
#include "renderer.h"

class EntityManager;

namespace graphics
{
    class VulkanRuntime;

    class SceneRenderer : public Renderer
    {
        VulkanRuntime& _vkr;
        EntityManager& _em;
        std::unordered_map<const Material*, VkPipeline> _cachedPipelines;

        void rebuild() override;

        VkPipeline getPipeline(const Material* mat);

        std::vector<GraphicsBuffer> _renderDataBuffers;
        std::vector<GraphicsBuffer> _pointLights;

        struct alignas(16) PointLightData
        {
            alignas(16) glm::vec3 position;
            alignas(16) glm::vec4 color;
        };

        struct alignas(16) RenderInfo
        {
            glm::mat4 render_matrix;
            glm::vec3 camera_pos;
        };

        void updateLights(size_t frame, std::vector<PointLightData>& lights);

      public:
        SceneRenderer(SwapChain& swapChain, VulkanRuntime* vkr, EntityManager* em);

        ~SceneRenderer() override;

        void render(VkCommandBuffer cmdBuffer) override;

        void reloadMaterial(Material* material);
    };
} // namespace graphics

#endif // BRANEENGINE_SCENERENDERER_H
