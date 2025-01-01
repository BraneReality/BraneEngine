#pragma once

#include "entityID.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "runtime/runtime.h"
#include "utility/inlineArray.h"

class InputSerializer;

class OutputSerializer;

class AssetID;

template<class T>
constexpr inline T* getVirtual(const uint8_t* var)
{
    return (T*)var;
}

template<class T>
constexpr inline T readVirtual(uint8_t* var)
{
    return *(T*)var;
}

template<class T>
constexpr inline T readVirtual(const uint8_t* var)
{
    return *(T*)var;
}

template<class T>
constexpr inline void setVirtual(const uint8_t* var, T value)
{
    *(T*)var = value;
}

namespace VirtualType
{
    enum Type
    {
        virtualUnknown = 0,
        virtualBool,
        virtualEntityID,
        virtualInt,
        virtualInt64,
        virtualUInt,
        virtualUInt64,
        virtualFloat,
        virtualString,
        virtualAssetID,
        virtualVec3,
        virtualVec4,
        virtualQuat,
        virtualMat4,
        virtualFloatArray,
        virtualIntArray,
        virtualUIntArray,
        virtualEntityIDArray
    };

    template<typename T>
    Type type();

    std::string typeToString(Type type);

    Type stringToType(const std::string& type);

    void serialize(Type type, OutputSerializer data, const uint8_t* source);

    void deserialize(Type type, InputSerializer data, uint8_t* source);

    size_t size(Type type);

    void construct(Type type, uint8_t* var);

    void deconstruct(Type type, uint8_t* var);

    void copy(Type type, uint8_t* dest, const uint8_t* source);

    void move(Type type, uint8_t* dest, uint8_t* source);

    template<typename T>
    void serialize(OutputSerializer& data, const uint8_t* source)
    {
        data << *getVirtual<T>(source);
    }

    template<typename T>
    void deserialize(InputSerializer& data, uint8_t* source)
    {
        data >> *getVirtual<T>(source);
    }

    template<typename T>
    void construct(uint8_t* var)
    {
        new(var) T();
    }

    template<typename T>
    void deconstruct(uint8_t* var)
    {
        ((T*)var)->~T();
    }

    template<typename T>
    void copy(uint8_t* dest, const uint8_t* source)
    {
        *((T*)dest) = *((T*)source);
    }

    template<typename T>
    void move(uint8_t* dest, uint8_t* source)
    {
        *((T*)dest) = std::move(*((T*)source));
    }
}; // namespace VirtualType

template<typename T>
VirtualType::Type VirtualType::type()
{
    if constexpr(std::is_same<T, bool>().value)
        return Type::virtualBool;
    if constexpr(std::is_same<T, EntityID>())
        return Type::virtualEntityID;
    if constexpr(std::is_same<T, int32_t>().value)
        return Type::virtualInt;
    if constexpr(std::is_same<T, uint32_t>().value)
        return Type::virtualUInt;
    if constexpr(std::is_same<T, int64_t>().value)
        return Type::virtualInt64;
    if constexpr(std::is_same<T, uint64_t>().value)
        return Type::virtualUInt64;
    if constexpr(std::is_same<T, float>().value)
        return Type::virtualFloat;
    if constexpr(std::is_same<T, std::string>().value)
        return Type::virtualString;
    if constexpr(std::is_same<T, AssetID>().value)
        return Type::virtualAssetID;
    if constexpr(std::is_same<T, glm::vec3>().value)
        return Type::virtualVec3;
    if constexpr(std::is_same<T, glm::vec4>().value)
        return Type::virtualVec4;
    if constexpr(std::is_same<T, glm::quat>().value)
        return Type::virtualQuat;
    if constexpr(std::is_same<T, glm::mat4>().value)
        return Type::virtualMat4;
    if constexpr(std::is_same<T, inlineFloatArray>().value)
        return Type::virtualFloatArray;
    if constexpr(std::is_same<T, inlineIntArray>().value)
        return Type::virtualIntArray;
    if constexpr(std::is_same<T, inlineUIntArray>().value)
        return Type::virtualUIntArray;
    if constexpr(std::is_same<T, inlineEntityIDArray>().value)
        return Type::virtualEntityIDArray;

    Runtime::error("Tried to find type of: [" + (std::string) typeid(T).name() + "] and failed");
    assert(false);
    return Type::virtualUnknown;
}
