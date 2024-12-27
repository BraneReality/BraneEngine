#include "meshAsset.h"
#include "graphics/graphics.h"
#include "runtime/runtime.h"

#include <utility/serializedData.h>

struct MeshSerializationContext : IncrementalAsset::SerializationContext
{
    size_t primitive;
    uint32_t pos;
    std::vector<bool> vertexSent;
};

void MeshAsset::serialize(OutputSerializer& s) const
{
    Asset::serialize(s);
    serializeHeader(s);
    s << _data;
}

void MeshAsset::deserialize(InputSerializer& s)
{
    Asset::deserialize(s);
    deserializeHeader(s);
    s >> _data;
}

MeshAsset::MeshAsset() { type.set(AssetType::Type::mesh); }

size_t MeshAsset::meshSize() const { return _data.size(); }

void MeshAsset::serializeHeader(OutputSerializer& s) const
{
    IncrementalAsset::serializeHeader(s);
    s << (uint16_t)_primitives.size();
    for(auto& primitive : _primitives)
    {
        s << primitive.indexOffset;
        s << primitive.indexType;
        s << primitive.indexCount;
        s << primitive.vertexCount;
        s << (uint16_t)primitive.attributes.size();
        for(auto& attribute : primitive.attributes)
        {
            s << attribute.first;
            s << attribute.second.offset;
            s << attribute.second.step;
        }
    }
    s << (uint32_t)_data.size();
}

void MeshAsset::deserializeHeader(InputSerializer& s)
{
    IncrementalAsset::deserializeHeader(s);
    uint16_t primitiveCount;
    s >> primitiveCount;
    _primitives.resize(primitiveCount);
    for(auto& primitive : _primitives)
    {
        s >> primitive.indexOffset;
        s >> primitive.indexType;
        s >> primitive.indexCount;
        s >> primitive.vertexCount;
        uint16_t attributeCount;
        s >> attributeCount;
        for(uint16_t i = 0; i < attributeCount; ++i)
        {
            std::pair<std::string, Primitive::Attribute> attribute;
            s >> attribute.first;
            s >> attribute.second.offset;
            s >> attribute.second.step;
            primitive.attributes.insert(attribute);
        }
    }
    uint32_t dataSize;
    s >> dataSize;
    _data.resize(dataSize);
}

// For now, we're just testing the header first, data later setup, so all meshes will be sent as only one increment.
bool MeshAsset::serializeIncrement(OutputSerializer& s, SerializationContext* iteratorData) const
{
    auto* itr = (MeshSerializationContext*)iteratorData;
    auto& primitive = _primitives[itr->primitive];
    s << (uint16_t)itr->primitive;

    uint32_t start = itr->pos;
    uint32_t end = std::min(primitive.indexCount, _trisPerIncrement * 3 + start);
    s << start << end;

    bool shortIndexType = primitive.indexType == Primitive::UInt16;

    for(size_t i = start; i < end; ++i)
    {
        uint32_t index;
        if(shortIndexType)
        {
            uint16_t shortIndex = *((uint16_t*)&_data[primitive.indexOffset + i * sizeof(uint16_t)]);
            s << shortIndex;
            index = shortIndex;
        }
        else
        {
            index = *((uint32_t*)&_data[primitive.indexOffset + i * sizeof(uint32_t)]);
            s << index;
        }

        s << !(bool)itr
                  ->vertexSent[index]; // We have to cast these because vector returns a custom wrapper for references
        // that's not "trivially copyable"
        // If we haven't sent this vertex, send it.
        if(!itr->vertexSent[index])
        {
            for(auto& a : primitive.attributes)
            {
                uint32_t attributeOffset = a.second.offset + a.second.step * index;
                s << a.second.step << attributeOffset;
                for(uint32_t j = 0; j < a.second.step; ++j)
                    s << (byte)_data[attributeOffset + j];
            }
            itr->vertexSent[index] = true;
        }
    }
    itr->pos = end;

    if(itr->pos == primitive.indexCount)
    {
        itr->primitive++;
        itr->pos = 0;

        if(itr->primitive >= _primitives.size())
            return false;
        itr->vertexSent.clear();
        itr->vertexSent.resize(_primitives[itr->primitive].vertexCount);
    }

    return true;
}

void MeshAsset::deserializeIncrement(InputSerializer& s)
{
    uint16_t pIndex;
    s >> pIndex;
    auto& primitive = _primitives[pIndex];

    bool shortIndexType = primitive.indexType == Primitive::UInt16;

    uint32_t start, end;
    s >> start >> end;

    for(size_t i = start; i < end; ++i)
    {
        size_t index;
        if(shortIndexType)
        {
            uint16_t sIndex;
            s >> sIndex;
            *((uint16_t*)&_data[primitive.indexOffset + sizeof(uint16_t) * i]) = sIndex;
            index = sIndex;
        }
        else
        {
            uint32_t uIndex;
            s >> uIndex;
            *((uint32_t*)&_data[primitive.indexOffset + sizeof(uint32_t) * i]) = uIndex;
            index = uIndex;
        }

        bool vertexSent;
        s >> vertexSent;
        // If we haven't received this vertex, save it.
        if(vertexSent)
        {
            for(int j = 0; j < primitive.attributes.size(); ++j)
            {
                uint32_t step, attributeOffset;
                s >> step >> attributeOffset;
                if(attributeOffset + step >= _data.size())
                    throw std::runtime_error("increment deserialization fail, out of bounds data");
                for(uint32_t k = 0; k < step; ++k)
                    s >> (byte&)_data[attributeOffset + k];
            }
        }
    }
#ifdef CLIENT
    meshUpdated = true;
#endif
}

std::unique_ptr<IncrementalAsset::SerializationContext> MeshAsset::createContext() const
{
    std::unique_ptr<MeshSerializationContext> sc = std::make_unique<MeshSerializationContext>();
    if(!_primitives.empty())
        sc->vertexSent.resize(_primitives[0].vertexCount);
    return std::move(sc);
}

size_t MeshAsset::addPrimitive(const std::vector<uint16_t>& indices, uint32_t vertexCount)
{
    size_t index = _data.size();
    Primitive p{};
    p.indexType = Primitive::UInt16;
    p.indexOffset = static_cast<uint32_t>(index);
    p.indexCount = static_cast<uint32_t>(indices.size());
    p.vertexCount = vertexCount;
    _primitives.push_back(p);

    size_t newSize = _data.size() + indices.size() * sizeof(uint16_t);
    newSize += 4 - newSize % 4;
    _data.resize(newSize);
    std::memcpy(&_data[index], indices.data(), indices.size() * sizeof(uint16_t));
    assert(_data.size() % 4 == 0);
    return _primitives.size() - 1;
}

size_t MeshAsset::addPrimitive(const std::vector<uint32_t>& indices, uint32_t vertexCount)
{
    size_t index = _data.size();
    Primitive p{};
    p.indexType = Primitive::UInt32;
    p.indexOffset = static_cast<uint32_t>(index);
    p.indexCount = static_cast<uint32_t>(indices.size());
    p.vertexCount = vertexCount;
    _primitives.push_back(p);

    size_t newSize = _data.size() + indices.size() * sizeof(uint32_t);
    _data.resize(newSize);
    std::memcpy(&_data[index], indices.data(), indices.size() * sizeof(uint32_t));
    assert(_data.size() % 4 == 0);
    return _primitives.size() - 1;
}

const std::vector<byte>& MeshAsset::packedData() const { return _data; }

uint32_t MeshAsset::indexOffset(size_t primitive) const
{
    assert(primitive < _primitives.size());
    return _primitives[primitive].indexOffset;
}

bool MeshAsset::hasAttribute(size_t primitive, const std::string& name) const
{
    assert(primitive < _primitives.size());
    return _primitives[primitive].attributes.count(name);
}

uint32_t MeshAsset::attributeOffset(size_t primitive, const std::string& name) const
{
    assert(primitive < _primitives.size());
    assert(_primitives[primitive].attributes.count(name));
    return _primitives[primitive].attributes.find(name)->second.offset;
}

size_t MeshAsset::primitiveCount() const { return _primitives.size(); }

uint32_t MeshAsset::vertexCount(uint32_t primitive) const
{
    assert(primitive < _primitives.size());
    return _primitives[primitive].vertexCount;
}

uint32_t MeshAsset::indexCount(size_t primitive) const
{
    assert(primitive < _primitives.size());
    return _primitives[primitive].indexCount;
}

#ifdef CLIENT
void MeshAsset::onDependenciesLoaded()
{
    auto* vkr = Runtime::getModule<graphics::VulkanRuntime>();
    if(runtimeID)
        runtimeID = vkr->addAsset(this);
}

MeshAsset::Primitive::IndexType MeshAsset::indexType(size_t primitive) const
{
    assert(primitive < _primitives.size());
    return _primitives[primitive].indexType;
}

#endif
