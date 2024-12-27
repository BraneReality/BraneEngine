#pragma once

#include <cstring>
#include <vector>
#include "../asset.h"
#include <glm/glm.hpp>

class MeshAsset : public IncrementalAsset
{
  public:
    uint32_t _trisPerIncrement = 60;

    struct Primitive
    {
        uint32_t indexOffset;

        enum IndexType : uint8_t
        {
            UInt16 = 0,
            UInt32 = 1
        } indexType;

        uint32_t indexCount;
        uint32_t vertexCount;

        struct Attribute
        {
            uint32_t offset;
            uint32_t step;
        };

        std::unordered_map<std::string, Attribute> attributes;
    };

  private:
    std::vector<Primitive> _primitives;
    std::vector<byte> _data;

  public:
    MeshAsset();

#ifdef CLIENT
    uint32_t runtimeID = -1;
    bool meshUpdated = false;
    void onDependenciesLoaded() override;
#endif

    size_t addPrimitive(const std::vector<uint16_t>& indices, uint32_t vertexCount);

    size_t addPrimitive(const std::vector<uint32_t>& indices, uint32_t vertexCount);

    template<typename T>
    void addAttribute(size_t primitive, const std::string& name, std::vector<T>& data)
    {
        assert(primitive < _primitives.size());
        size_t index = _data.size();
        size_t newSize = _data.size() + data.size() * sizeof(T);
        newSize += 4 - newSize % 4;
        _data.resize(newSize);
        std::memcpy(&_data[index], data.data(), data.size() * sizeof(T));
        assert(_data.size() % 4 == 0);

        _primitives[primitive].attributes.insert({name, {(uint32_t)index, sizeof(T)}});
    }

    const std::vector<byte>& packedData() const;

    uint32_t indexOffset(size_t primitive) const;

    uint32_t indexCount(size_t primitive) const;

    Primitive::IndexType indexType(size_t primitive) const;

    bool hasAttribute(size_t primitive, const std::string& name) const;

    uint32_t attributeOffset(size_t primitive, const std::string& name) const;

    uint32_t vertexCount(uint32_t primitive) const;

    void serialize(OutputSerializer& s) const override;

    void deserialize(InputSerializer& s) override;

    void serializeHeader(OutputSerializer& s) const override;

    void deserializeHeader(InputSerializer& s) override;

    std::unique_ptr<SerializationContext> createContext() const override;

    bool serializeIncrement(OutputSerializer& sData, SerializationContext* iteratorData) const override;

    void deserializeIncrement(InputSerializer& sData) override;

    size_t meshSize() const;

    size_t primitiveCount() const;
};