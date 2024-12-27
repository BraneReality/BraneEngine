#pragma once

#include <cassert>
#include <cstring>
#include <vector>
#include "graphicsDevice.h"
#include <GLFW/glfw3.h>

namespace graphics
{
    class SingleUseCommandBuffer
    {
        VkCommandBuffer _buffer;
        VkCommandPool _pool;
        bool _submitted;

      public:
        SingleUseCommandBuffer(VkCommandPool pool);

        ~SingleUseCommandBuffer();

        VkCommandBuffer get();

        void submit(VkQueue submitQueue);
    };

    class GraphicsBuffer
    {
        VkBuffer _buffer{};
        VkDeviceMemory _memory{};
        size_t _size;
        VkBufferUsageFlags _usageFlags;
        VkBufferUsageFlags _memFlags;

      public:
        GraphicsBuffer();

        GraphicsBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags);

        ~GraphicsBuffer();

        VkBuffer get() const;

        size_t size() const;

        void setFlags(VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags);

        void setData(const void* data, VkDeviceSize size, VkDeviceSize startIndex);

        template<class T>
        void setData(const T& data, VkDeviceSize startIndex = 0)
        {
            assert(startIndex >= 0);
            assert(startIndex + sizeof(T) <= _size * sizeof(T));
            setData(&data, sizeof(T), startIndex);
        }

        template<class T>
        void setData(const std::vector<T>& data, VkDeviceSize startIndex = 0)
        {
            assert(startIndex >= 0);
            assert(startIndex + data.size() * sizeof(T) <= _size);
            setData(data.data(), data.size() * sizeof(T), startIndex);
        }

        void copy(GraphicsBuffer* src,
                  VkCommandBuffer commandBuffer,
                  VkDeviceSize size,
                  VkDeviceSize srcOffset = 0,
                  VkDeviceSize dstOffset = 0);

        void realocate(VkDeviceSize newSize);
    };
} // namespace graphics