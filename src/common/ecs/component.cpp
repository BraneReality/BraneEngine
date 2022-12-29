#include "component.h"
#include <algorithm>
#include <cstring>


#include "linker.h"
#include "utility/serializedData.h"

ComponentDescription::ComponentDescription(const BraneScript::StructDef* type, const BraneScript::Linker* linker)
{
    _type = type;
    _constructor = (void (*)(byte*))linker->getFunction(std::string(type->name()) + "::construct()");
    _destructor  = (void (*)(byte*))linker->getFunction(std::string(type->name()) + "::destruct()");
    _copy = (void (*)(byte*, const byte*))linker->getFunction(std::string(type->name()) + "::copy(const ref " + std::string(type->name()) + ")");
    _move = (void (*)(byte*, byte*))linker->getFunction(std::string(type->name()) + "::move(ref " + std::string(type->name()) + ")");
}

void ComponentDescription::construct(byte* component) const
{
    _constructor(component);
}

void ComponentDescription::deconstruct(byte* component) const
{
    _destructor(component);
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

void ComponentDescription::copy(const byte* src, byte* dest) const
{
    _copy(dest, src);
}

void ComponentDescription::move(byte* src, byte* dest) const
{
    _move(dest, src);
}

size_t ComponentDescription::size() const
{
    return _type->size();
}

VirtualComponent::VirtualComponent(const VirtualComponent& source)
{
    _description = source._description;
    _data = new byte[_description->size()];
    _description->construct(_data);
}
VirtualComponent::VirtualComponent(const VirtualComponentView& source)
{
    _description = source.description();
    _data = new byte[_description->size()];
    _description->construct(_data);
    _description->copy(source.data(), _data);
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
    _description->construct(_data);
}

VirtualComponent::VirtualComponent(const ComponentDescription* definition, const byte* data)
{
    _description = definition;
    _data = new byte[_description->size()];
    _description->construct(_data);
    _description->copy(data, _data);
}

VirtualComponent::~VirtualComponent()
{
    if(_data)
    {
        _description->deconstruct(_data);
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
            _description->deconstruct(_data);
        if(_description->size() != source._description->size())
        {
            if(_data)
                delete _data;
            _data = new byte[source._description->size()];
        }
        _description = source._description;
        _description->construct(_data);
    }

    _description->copy(source.data(), _data);
    return *this;
}

VirtualComponent& VirtualComponent::operator=(const VirtualComponentView& source)
{
    if(source.data() == _data)
        return *this;

    if(_description != source.description() || !_data)
    {
        if(_data)
            _description->deconstruct(_data);
        if(_description->size() != source.description()->size())
        {
            if(_data)
                delete _data;
            _data = new byte[source.description()->size()];
        }
        _description = source.description();
        _description->construct(_data);
    }

    _description->copy(source.data(), _data);
    return *this;
}


byte* VirtualComponent::data() const
{
    return _data;
}

const ComponentDescription* VirtualComponent::description() const
{
    return _description;
}

VirtualComponentView::VirtualComponentView(const VirtualComponent& source) : VirtualComponentView(source.description(), source.data())
{

}

VirtualComponentView::VirtualComponentView(const ComponentDescription* description, byte* data)
{
    _data = data;
    _description = description;
}

byte* VirtualComponentView::data() const
{
    return _data;
}

const ComponentDescription* VirtualComponentView::description() const
{
    return _description;
}
