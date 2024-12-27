#include "mesh.h"
#include "common/ecs/component.h"
#include "graphicsBuffer.h"
#include "utility/serializedData.h"
#include <assets/types/meshAsset.h>

namespace graphics
{
    Mesh::Mesh(MeshAsset* meshAsset)
    {
        _meshAsset = meshAsset;
        _locked = true;
        unlock();

        _stagingBuffer->setData(_meshAsset->packedData(), 0);

        _dataBuffer = new GraphicsBuffer(size(),
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        SingleUseCommandBuffer cmdBuffer(device->transferPool());
        _dataBuffer->copy(_stagingBuffer, cmdBuffer.get(), size());
        cmdBuffer.submit(device->transferQueue());
    }

    Mesh::~Mesh()
    {
        if(!_locked)
            delete _stagingBuffer;
        delete _dataBuffer;
    }

    uint32_t Mesh::size() const { return _meshAsset->meshSize(); }

    void Mesh::lock()
    {
        if(!_locked)
        {
            delete _stagingBuffer;
            _locked = false;
        }
    }

    void Mesh::unlock()
    {
        if(_locked)
        {
            _stagingBuffer =
                new GraphicsBuffer(size(),
                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            _locked = false;
        }
    }

    uint32_t Mesh::vertexCount(uint32_t primitive) const { return _meshAsset->vertexCount(primitive); }

    VkIndexType Mesh::indexBufferType(uint32_t primitive) const
    {
        return (_meshAsset->indexType(primitive) == MeshAsset::Primitive::UInt16) ? VK_INDEX_TYPE_UINT16
                                                                                  : VK_INDEX_TYPE_UINT32;
    }

    VkDeviceSize Mesh::indexBufferOffset(uint32_t primitive) const { return _meshAsset->indexOffset(primitive); }

    uint32_t Mesh::primitiveCount() const { return _meshAsset->primitiveCount(); }

    MeshAsset* Mesh::meshAsset() { return _meshAsset; }

    void Mesh::updateData()
    {
        if(!_meshAsset->meshUpdated)
            return;

        _stagingBuffer->setData(_meshAsset->packedData(), 0);

        SingleUseCommandBuffer cmdBuffer(device->transferPool());
        _dataBuffer->copy(_stagingBuffer, cmdBuffer.get(), size());
        cmdBuffer.submit(device->transferQueue());

        _meshAsset->meshUpdated = false;
    }

    bool Mesh::hasAttributeBuffer(uint32_t primitive, const std::string& name) const
    {
        return _meshAsset->hasAttribute(primitive, name);
    }

    VkDeviceSize Mesh::attributeBufferOffset(uint32_t primitive, const std::string& name) const
    {
        return _meshAsset->attributeOffset(primitive, name);
    }

    VkBuffer Mesh::buffer() const { return _dataBuffer->get(); }

    uint32_t Mesh::indexCount(uint32_t primitive) const { return _meshAsset->indexCount(primitive); }

} // namespace graphics