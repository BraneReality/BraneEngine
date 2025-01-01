//
// Created by eli on 7/17/2022.
//

#ifndef BRANEENGINE_NATIVECOMPONENT_H
#define BRANEENGINE_NATIVECOMPONENT_H

#include "component.h"
#include "structMembers.h"

template<class T>
class NativeComponent
{
  protected:
    using ComponentType = T;
    static ComponentDescription* _description;

  public:
    static ComponentDescription* constructDescription()
    {
        if(_description != nullptr)
            return _description;
        AssetID id;
        std::vector<VirtualType::Type> members = T::getMemberTypes();
        std::vector<size_t> offsets = T::getMemberOffsets();
        _description = new ComponentDescription(members, offsets, sizeof(T));
        _description->name = T::getComponentName();
        return _description;
    }

    NativeComponent() = default;

    VirtualComponentView toVirtual() const
    {
        return VirtualComponentView(_description, (uint8_t*)this);
    }

    operator VirtualComponentView() const
    {
        return toVirtual();
    }

    static T* fromVirtual(uint8_t* data)
    {
        return (T*)data;
    }

    static const T* fromVirtual(const uint8_t* data)
    {
        return (const T*)data;
    }

    static T* fromVirtual(VirtualComponentView component)
    {
        return (T*)component.data();
    }

    static ComponentDescription* def()
    {
        return _description;
    }
};

template<class T>
ComponentDescription* NativeComponent<T>::_description = nullptr;

#endif // BRANEENGINE_NATIVECOMPONENT_H
