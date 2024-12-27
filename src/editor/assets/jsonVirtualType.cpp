//
// Created by eli on 8/20/2022.
//

#include "jsonVirtualType.h"
#include "assets/assetID.h"
#include "utility/jsonTypeUtilities.h"

Json::Value JsonVirtualType::fromVirtual(byte* data, VirtualType::Type type)
{
    switch(type)
    {
        case VirtualType::virtualUnknown:
            return Json::nullValue;
        case VirtualType::virtualEntityID:
            return getVirtual<EntityID>(data)->id;
        case VirtualType::virtualBool:
            return *getVirtual<bool>(data);
        case VirtualType::virtualInt:
            return *getVirtual<int32_t>(data);
        case VirtualType::virtualInt64:
            return *getVirtual<int64_t>(data);
        case VirtualType::virtualUInt:
            return *getVirtual<uint32_t>(data);
        case VirtualType::virtualUInt64:
            return *getVirtual<uint64_t>(data);
        case VirtualType::virtualFloat:
            return *getVirtual<float>(data);
        case VirtualType::virtualString:
            return *getVirtual<std::string>(data);
        case VirtualType::virtualAssetID:
            return getVirtual<AssetID>(data)->string();
        case VirtualType::virtualVec3:
            return toJson(*getVirtual<glm::vec3>(data));
        case VirtualType::virtualVec4:
            return toJson(*getVirtual<glm::vec4>(data));
        case VirtualType::virtualQuat:
            return toJson(*getVirtual<glm::quat>(data));
        case VirtualType::virtualMat4:
            return toJson(*getVirtual<glm::mat4>(data));
        case VirtualType::virtualFloatArray:
            return toJson(*getVirtual<inlineFloatArray>(data));
        case VirtualType::virtualIntArray:
            return toJson(*getVirtual<inlineIntArray>(data));
        case VirtualType::virtualUIntArray:
            return toJson(*getVirtual<inlineUIntArray>(data));
        case VirtualType::virtualEntityIDArray:
            return toJson(*getVirtual<inlineEntityIDArray>(data));
        default:
            Runtime::error("Can't convert " + VirtualType::typeToString(type) + " to json!");
            assert(false);
            return Json::nullValue;
    }
}

void JsonVirtualType::toVirtual(byte* data, const Json::Value& source, VirtualType::Type type)
{
    switch(type)
    {
        case VirtualType::virtualEntityID:
            *getVirtual<EntityID>(data) = {source.asUInt(), 0};
            break;
        case VirtualType::virtualBool:
            *getVirtual<bool>(data) = source.asBool();
            break;
        case VirtualType::virtualInt:
            *getVirtual<int32_t>(data) = source.asInt();
            break;
        case VirtualType::virtualInt64:
            *getVirtual<int64_t>(data) = source.asInt64();
            break;
        case VirtualType::virtualUInt:
            *getVirtual<uint32_t>(data) = source.asUInt();
            break;
        case VirtualType::virtualUInt64:
            *getVirtual<uint64_t>(data) = source.asUInt64();
            break;
        case VirtualType::virtualFloat:
            *getVirtual<float>(data) = source.asFloat();
            break;
        case VirtualType::virtualString:
            *getVirtual<std::string>(data) = source.asString();
            break;
        case VirtualType::virtualAssetID:
            *getVirtual<AssetID>(data) = source.asString();
            break;
        case VirtualType::virtualVec3:
            *getVirtual<glm::vec3>(data) = fromJson<glm::vec3>(source);
            break;
        case VirtualType::virtualVec4:
            *getVirtual<glm::vec4>(data) = fromJson<glm::vec4>(source);
            break;
        case VirtualType::virtualQuat:
            *getVirtual<glm::quat>(data) = fromJson<glm::quat>(source);
            break;
        case VirtualType::virtualMat4:
            *getVirtual<glm::mat4>(data) = fromJson<glm::mat4>(source);
            break;
        case VirtualType::virtualFloatArray:
            *getVirtual<inlineFloatArray>(data) = fromJson<inlineFloatArray>(source);
            break;
        case VirtualType::virtualIntArray:
            *getVirtual<inlineIntArray>(data) = fromJson<inlineIntArray>(source);
            break;
        case VirtualType::virtualUIntArray:
            *getVirtual<inlineUIntArray>(data) = fromJson<inlineUIntArray>(source);
            break;
        case VirtualType::virtualEntityIDArray:
            *getVirtual<inlineEntityIDArray>(data) = fromJson<inlineEntityIDArray>(data);
            break;
        default:
            Runtime::error("Can't convert " + VirtualType::typeToString(type) + " to json!");
            assert(false);
    }
}