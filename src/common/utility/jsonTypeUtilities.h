//
// Created by eli on 8/18/2022.
//

#ifndef BRANEENGINE_JSONTYPEUTILITIES_H
#define BRANEENGINE_JSONTYPEUTILITIES_H

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/gtx/quaternion.hpp"
#include <json/json.h>

template<typename T>
T fromJson(const Json::Value& value)
{
    if constexpr(std::is_same<T, glm::mat4>())
    {
        glm::mat4 mat(1);
        for(Json::ArrayIndex i = 0; i < value.size(); ++i)
            mat[i / 4][i % 4] = value[i].asFloat();
        return mat;
    }
    else if constexpr(std::is_same<T, glm::vec2>())
    {
        glm::vec2 vec;
        for(Json::ArrayIndex i = 0; i < value.size(); ++i)
            vec[i] = value[i].asFloat();
        return vec;
    }
    else if constexpr(std::is_same<T, glm::vec3>())
    {
        glm::vec3 vec;
        for(Json::ArrayIndex i = 0; i < value.size(); ++i)
            vec[i] = value[i].asFloat();
        return vec;
    }
    else if constexpr(std::is_same<T, glm::vec4>())
    {
        glm::vec4 vec;
        for(Json::ArrayIndex i = 0; i < value.size(); ++i)
            vec[i] = value[i].asFloat();
        return vec;
    }
    else if constexpr(std::is_same<T, glm::quat>())
    {
        glm::quat quat;

        quat.w = value[3].asFloat();
        quat.x = value[0].asFloat();
        quat.y = value[1].asFloat();
        quat.z = value[2].asFloat();
        return quat;
    }
    else if constexpr(std::is_same<T, inlineEntityIDArray>())
    {
        inlineEntityIDArray array;
        for(auto& m : value)
            array.push_back({m.asUInt(), 0});
        return array;
    }
    else if constexpr(std::is_same<T, inlineFloatArray>())
    {
        inlineFloatArray array;
        for(auto& m : value)
            array.push_back(m.asFloat());
        return array;
    }
    else if constexpr(std::is_same<T, inlineIntArray>())
    {
        inlineIntArray array;
        for(auto& m : value)
            array.push_back(m.asInt());
        return array;
    }
    else if constexpr(std::is_same<T, inlineUIntArray>())
    {
        inlineUIntArray array;
        for(auto& m : value)
            array.push_back(m.asUInt());
        return array;
    }
    else
    {
        return value.as<T>();
    }
}

template<typename T>
Json::Value toJson(const T& value)
{
    Json::Value json;
    if constexpr(std::is_same<T, glm::mat4>())
    {
        for(Json::ArrayIndex i = 0; i < 16; ++i)
            json.append(value[i / 4][i % 4]);
    }
    else if constexpr(std::is_same<T, glm::vec2>())
    {
        for(Json::ArrayIndex i = 0; i < 3; ++i)
            json.append(value[i]);
    }
    else if constexpr(std::is_same<T, glm::vec3>())
    {
        for(Json::ArrayIndex i = 0; i < 3; ++i)
            json.append(value[i]);
    }
    else if constexpr(std::is_same<T, glm::vec4>())
    {
        for(Json::ArrayIndex i = 0; i < 4; ++i)
            json.append(value[i]);
    }
    else if constexpr(std::is_same<T, glm::quat>())
    {
        json.append(value.x);
        json.append(value.y);
        json.append(value.z);
        json.append(value.w);
    }
    else if constexpr(std::is_same<T, inlineEntityIDArray>())
    {
        for(auto& m : value)
            json.append(m.id);
    }
    else if constexpr(std::is_same<T, inlineFloatArray>())
    {
        for(auto& m : value)
            json.append(m);
    }
    else if constexpr(std::is_same<T, inlineIntArray>())
    {
        for(auto& m : value)
            json.append(m);
    }
    else if constexpr(std::is_same<T, inlineUIntArray>())
    {
        for(auto& m : value)
            json.append(m);
    }
    else
        json = value;
    return json;
}

#endif // BRANEENGINE_JSONTYPEUTILITIES_H
