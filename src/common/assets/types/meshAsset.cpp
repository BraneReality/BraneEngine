#include "meshAsset.h"

#include <utility/serializedData.h>

void MeshAsset::serialize(OutputSerializer s)
{
	Asset::serialize(s);
    serializeHeader(s);
    s << _data;
}

void MeshAsset::deserialize(InputSerializer s)
{
	Asset::deserialize(s);
    deserializeHeader(s);
    s >> _data;
}

MeshAsset::MeshAsset()
{
	pipelineID = -1;
	type.set(AssetType::Type::mesh);
	meshUpdated = false;
}

size_t MeshAsset::meshSize() const
{
	return _data.size();
}
void MeshAsset::serializeHeader(OutputSerializer s)
{
	IncrementalAsset::serializeHeader(s);
    s << (uint16_t)_primitives.size();
    for(auto& primitive : _primitives)
    {
        s << primitive.indexOffset;
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

void MeshAsset::deserializeHeader(InputSerializer s)
{
	IncrementalAsset::deserializeHeader(s);
    uint16_t primitiveCount;
    s >> primitiveCount;
    _primitives.resize(primitiveCount);
    for(auto& primitive : _primitives)
    {
        s >> primitive.indexOffset;
        s >> primitive.indexCount;
        s >> primitive.vertexCount;
        uint16_t attributeCount;
        s >> attributeCount;
        for (uint16_t i = 0; i < attributeCount; ++i)
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
	loadState = partial;
}

//For now, we're just testing the header first, data later setup, so all meshes will be sent as only one increment.
bool MeshAsset::serializeIncrement(OutputSerializer s, SerializationContext* iteratorData)
{
	auto* itr = (MeshSerializationContext*)iteratorData;
	auto& primitive = _primitives[itr->primitive];
	s << (uint16_t)itr->primitive;

	uint32_t start = itr->pos;
	uint32_t end = std::min(primitive.indexCount, _trisPerIncrement * 3 + start);
	s << start << end;

	for (size_t i = start; i < end; ++i)
	{
		uint16_t index = *((uint16_t*)&_data[primitive.indexOffset + i * sizeof(uint16_t)]);
		s << index;
		s << !(bool)itr->vertexSent[index]; //We have to cast these because vector returns a custom wrapper for references that's not "trivially copyable"
		// If we haven't sent this vertex, send it.
		if(!itr->vertexSent[index])
		{
            for(auto& a : primitive.attributes)
            {
                size_t attributeOffset = a.second.offset + a.second.step * index;
                s << a.second.step;
                s << attributeOffset;
                for (uint32_t j = 0; j < a.second.step; ++j)
                {
                    s << (byte)_data[attributeOffset + j];
                }
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

void MeshAsset::deserializeIncrement(InputSerializer s)
{
	uint16_t pIndex;
	s >> pIndex;
	auto& primitive = _primitives[pIndex];

	uint32_t start, end;
	s >> start >> end;

    for (size_t i = start; i < end; ++i)
    {
        uint16_t index;
        s >> index;
        *((uint16_t*)&_data[primitive.indexOffset + 2 * i]) = index;
        bool vertexSent;
        s >> vertexSent; //We have to cast these because vector returns a custom wrapper for references that's not "trivially copyable"
        // If we haven't received this vertex, save it.
        if(vertexSent)
        {
            for (int j = 0; j < primitive.attributes.size(); ++j)
            {
                uint32_t step;
                s >> step;
                size_t attributeOffset;
                s >> attributeOffset;
                if(attributeOffset + step >= _data.size())
                    throw std::runtime_error("increment deserialization fail, out of bounds data");
                for (uint32_t j = 0; j < step; ++j)
                {
                    s >> (byte&)_data[attributeOffset + j];
                }
            }
        }
    }
	meshUpdated = true;
}

std::unique_ptr<IncrementalAsset::SerializationContext> MeshAsset::createContext() const
{
    std::unique_ptr<MeshSerializationContext> sc = std::make_unique<MeshSerializationContext>();
    if(!_primitives.empty());
    sc->vertexSent.resize(_primitives[0].vertexCount);
    return std::move(sc);
}

size_t MeshAsset::addPrimitive(const std::vector<uint16_t>& indices, uint32_t vertexCount)
{
    size_t index = _data.size();
    Primitive p{};
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

const std::vector<byte>& MeshAsset::packedData() const
{
    return _data;
}

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

size_t MeshAsset::primitiveCount() const
{
    return _primitives.size();
}

uint32_t MeshAsset::vertexCount(uint32_t primitive) const
{
    return _primitives[primitive].vertexCount;
}

uint32_t MeshAsset::indexCount(size_t primitive) const
{
    return _primitives[primitive].indexCount;
}


