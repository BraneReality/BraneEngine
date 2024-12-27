#include "component.h"
#include <algorithm>
#include <cstring>

#include "common/assets/types/componentAsset.h"
#include "utility/serializedData.h"

ComponentDescription::ComponentDescription(const ComponentAsset* asset) : ComponentDescription(asset->members())
{
    name = asset->name;
    this->asset = asset;
}

ComponentDescription::ComponentDescription(const std::vector<VirtualType::Type>& members)
    : ComponentDescription(members, generateOffsets(members))
{}

ComponentDescription::ComponentDescription(const std::vector<VirtualType::Type>& members,
                                           const std::vector<size_t>& offsets,
                                           size_t size)
    : ComponentDescription(members, offsets)
{
    _size = size;
}

ComponentDescription::ComponentDescription(const std::vector<VirtualType::Type>& members,
                                           const std::vector<size_t>& offsets)
{
    _members.resize(members.size());
    for(size_t i = 0; i < members.size(); i++)
    {
        _members[i].type = members[i];
        _members[i].offset = offsets[i];
    }
}

std::vector<size_t> ComponentDescription::generateOffsets(const std::vector<VirtualType::Type>& members)
{
    std::vector<size_t> offsets(members.size());
    _size = 0;
    for(size_t i = 0; i < members.size(); ++i)
    {
        size_t typesize = VirtualType::size(members[i]);

        size_t woffset = _size % WORD_SIZE;
        if(woffset != 0 && WORD_SIZE - woffset < typesize)
        {
            _size += WORD_SIZE - woffset;
        }
        offsets[i] = _size;
        _size += typesize;
    }

    return offsets;
}

void ComponentDescription::construct(byte* component) const
{
    for(auto& m : _members)
    {
        VirtualType::construct(m.type, component + m.offset);
    }
}

void ComponentDescription::deconstruct(byte* component) const
{
    for(auto& m : _members)
    {
        VirtualType::deconstruct(m.type, component + m.offset);
    }
}

void ComponentDescription::serialize(OutputSerializer& sData, byte* component) const
{
    for(auto& m : _members)
    {
        VirtualType::serialize(m.type, sData, component + m.offset);
    }
}

void ComponentDescription::deserialize(InputSerializer& sData, byte* component) const
{
    for(auto& m : _members)
    {
        VirtualType::deserialize(m.type, sData, component + m.offset);
    }
}

void ComponentDescription::copy(byte* src, byte* dest) const
{
    for(auto& m : _members)
    {
        VirtualType::copy(m.type, dest + m.offset, src + m.offset);
    }
}

void ComponentDescription::move(byte* src, byte* dest) const
{
    for(auto& m : _members)
    {
        VirtualType::move(m.type, dest + m.offset, src + m.offset);
    }
}

const std::vector<ComponentDescription::Member>& ComponentDescription::members() const { return _members; }

size_t ComponentDescription::size() const { return _size; }

size_t ComponentDescription::serializationSize() const
{
    size_t ss = 0;
    for(auto& m : _members)
        ss += VirtualType::size(m.type);
    return ss;
}

VirtualComponent::VirtualComponent(const VirtualComponent& source)
{
    _description = source._description;
    _data = new byte[_description->size()];
    for(auto& member : _description->members())
    {
        VirtualType::construct(member.type, _data + member.offset);
        VirtualType::copy(member.type, _data + member.offset, source._data + member.offset);
    }
}

VirtualComponent::VirtualComponent(const VirtualComponentView& source)
{
    _description = source.description();
    _data = new byte[_description->size()];
    for(auto& member : _description->members())
    {
        VirtualType::construct(member.type, _data + member.offset);
        VirtualType::copy(member.type, _data + member.offset, source.data() + member.offset);
    }
}

VirtualComponent::VirtualComponent(VirtualComponent&& source)
{
    _description = source._description;
    _data = source._data;
    source._data = nullptr;
}

VirtualComponent::VirtualComponent(const ComponentDescription* definition)
{
    _description = definition;
    _data = new byte[_description->size()];
    for(auto& member : _description->members())
    {
        VirtualType::construct(member.type, _data + member.offset);
    }
}

VirtualComponent::VirtualComponent(const ComponentDescription* definition, const byte* data)
{
    _description = definition;
    _data = new byte[_description->size()];
    for(auto& member : _description->members())
    {
        VirtualType::construct(member.type, _data + member.offset);
        VirtualType::copy(member.type, _data + member.offset, data + member.offset);
    }
}

VirtualComponent::~VirtualComponent()
{
    if(_data)
    {
        for(auto& member : _description->members())
        {
            VirtualType::deconstruct(member.type, _data + member.offset);
        }
        delete[] _data;
    }
}

VirtualComponent& VirtualComponent::operator=(const VirtualComponent& source)
{
    if(source._data == _data)
        return *this;

    if(_description != source._description || !_data)
    {
        if(_data)
        {
            for(auto& member : _description->members())
                VirtualType::deconstruct(member.type, _data + member.offset);
        }
        if(_description->size() != source._description->size())
        {
            if(_data)
                delete _data;
            _data = new byte[source._description->size()];
        }
        _description = source._description;
        for(auto& member : _description->members())
            VirtualType::construct(member.type, _data + member.offset);
    }

    for(auto& member : _description->members())
        VirtualType::copy(member.type, _data + member.offset, source._data + member.offset);
    return *this;
}

VirtualComponent& VirtualComponent::operator=(const VirtualComponentView& source)
{
    if(source.data() == _data)
        return *this;

    if(_description != source.description() || !_data)
    {
        if(_data)
        {
            for(auto& member : _description->members())
                VirtualType::deconstruct(member.type, _data + member.offset);
        }
        if(_description->size() != source.description()->size())
        {
            if(_data)
                delete _data;
            _data = new byte[source.description()->size()];
        }
        _description = source.description();
        for(auto& member : _description->members())
            VirtualType::construct(member.type, _data + member.offset);
    }

    for(auto& member : _description->members())
        VirtualType::copy(member.type, _data + member.offset, source.data() + member.offset);
    return *this;
}

byte* VirtualComponent::data() const { return _data; }

const ComponentDescription* VirtualComponent::description() const { return _description; }

VirtualComponentView::VirtualComponentView(const VirtualComponent& source)
    : VirtualComponentView(source.description(), source.data())
{}

VirtualComponentView::VirtualComponentView(const ComponentDescription* description, byte* data)
{
    _data = data;
    _description = description;
}

byte* VirtualComponentView::data() const { return _data; }

const ComponentDescription* VirtualComponentView::description() const { return _description; }
