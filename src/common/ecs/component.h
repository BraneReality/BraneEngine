#pragma once

#include "virtualType.h"

#include <cassert>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include "common/utility/staticIndexVector.h"
#include <unordered_set>

#ifdef _64BIT
#define WORD_SIZE 8
#endif
#ifdef _32BIT
#define WORD_SIZE 4
#endif

class ComponentAsset;

using ComponentID = uint32_t;

class ComponentDescription
{
    struct Member
    {
        VirtualType::Type type;
        size_t offset;
    };

    std::vector<Member> _members;
    size_t _size;

    std::vector<size_t> generateOffsets(const std::vector<VirtualType::Type>&);

  public:
    ComponentID id;
    std::string name;
    const ComponentAsset* asset = nullptr;

    ComponentDescription(const ComponentAsset* asset);

    ComponentDescription(const std::vector<VirtualType::Type>& members);

    ComponentDescription(const std::vector<VirtualType::Type>& members, const std::vector<size_t>& offsets);

    ComponentDescription(const std::vector<VirtualType::Type>& members,
                         const std::vector<size_t>& offsets,
                         size_t size);

    void construct(byte* component) const;

    void deconstruct(byte* component) const;

    void serialize(OutputSerializer& sData, byte* component) const;

    void deserialize(InputSerializer& sData, byte* component) const;

    void copy(byte* src, byte* dest) const;

    void move(byte* src, byte* dest) const;

    const std::vector<Member>& members() const;

    size_t size() const;

    size_t serializationSize() const;
};

class VirtualComponentView;

class VirtualComponent
{
  protected:
    byte* _data;
    const ComponentDescription* _description;

  public:
    VirtualComponent(const VirtualComponent& source);

    VirtualComponent(const VirtualComponentView& source);

    VirtualComponent(VirtualComponent&& source);

    VirtualComponent(const ComponentDescription* definition);

    VirtualComponent(const ComponentDescription* definition, const byte* data);

    ~VirtualComponent();

    VirtualComponent& operator=(const VirtualComponent& source);

    VirtualComponent& operator=(const VirtualComponentView& source);

    template<class T>
    T* getVar(size_t index) const
    {
        assert(index < _description->members().size());
        assert(_description->members()[index].offset + sizeof(T) <= _description->size());
        return getVirtual<T>(&_data[_description->members()[index].offset]);
    }

    template<class T>
    void setVar(size_t index, T value)
    {
        assert(index < _description->members().size());
        assert(_description->members()[index].offset + sizeof(T) <= _description->size());
        *(T*)&_data[_description->members()[index].offset] = value;
    }

    template<class T>
    T readVar(size_t index) const
    {
        assert(index < _description->members().size());
        assert(_description->members()[index].offset + sizeof(T) <= _description->size());
        return *(T*)&_data[_description->members()[index].offset];
    }

    byte* data() const;

    const ComponentDescription* description() const;
};

class VirtualComponentView
{
  protected:
    byte* _data;
    const ComponentDescription* _description;

  public:
    VirtualComponentView(const VirtualComponent& source);

    VirtualComponentView(const ComponentDescription* description, byte* data);

    template<class T>
    T* getVar(size_t index) const
    {
        assert(index < _description->members().size());
        assert(_description->members()[index].offset + sizeof(T) <= _description->size());
        return getVirtual<T>(&_data[_description->members()[index].offset]);
    }

    template<class T>
    void setVar(size_t index, T value)
    {
        assert(index < _description->members().size());
        assert(_description->members()[index].offset + sizeof(T) <= _description->size());
        *(T*)&_data[_description->members()[index].offset] = value;
    }

    template<class T>
    T readVar(size_t index) const
    {
        assert(index < _description->members().size());
        assert(_description->members()[index].offset + sizeof(T) <= _description->size());
        return *(T*)&_data[_description->members()[index].offset];
    }

    byte* data() const;

    const ComponentDescription* description() const;
};
