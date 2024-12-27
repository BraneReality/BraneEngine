#pragma once

#include <GLFW/glfw3.h>

#include <array>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

class MeshAsset;

namespace graphics
{
    class GraphicsBuffer;

    using MeshID = uint32_t;

    class Mesh
    {
      private:
        bool _locked;
        GraphicsBuffer* _stagingBuffer;
        GraphicsBuffer* _dataBuffer;
        MeshAsset* _meshAsset;

      public:
        Mesh(MeshAsset* meshAsset);

        ~Mesh();

        MeshAsset* meshAsset();

        VkBuffer buffer() const;

        VkIndexType indexBufferType(uint32_t primitive) const;

        VkDeviceSize indexBufferOffset(uint32_t primitive) const;

        bool hasAttributeBuffer(uint32_t, const std::string& name) const;

        VkDeviceSize attributeBufferOffset(uint32_t primitive, const std::string& name) const;

        uint32_t size() const;

        uint32_t indexCount(uint32_t primitive) const;

        uint32_t vertexCount(uint32_t primitive) const;

        uint32_t primitiveCount() const;

        void updateData();

        void lock();

        void unlock();
    };
} // namespace graphics