#pragma once
#include <format>
#include <string>
#include <typeinfo>
#include "glm/glm.hpp"
#include "result.h"
#include "runtime/runtime.h"
#include <json/json.h>

struct JsonSerializerError
{
    enum Type
    {
        MissingKey,
        WrongType,
        WrongStringFormat,
        ParserError
    } type;

    std::string message;

    inline JsonSerializerError(Type type, std::string message) : type(type), message(message) {}

    std::string toString() const;
};

template<typename T>
struct JsonSerializer
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, T& value)
    {
        static_assert(false, "No default implementation of JsonSerializer<T>::read");
        throw std::runtime_error(std::format("No implementation of JsonSerializer<{}>", typeid(T).name()));
        // static_assert(false, "No Implementation of JsonSerializer<T>::read for this type!");
        // commented out static_assert cause it wasn't giving me type names, "it's broken!" cool, how? TELL ME. At least
        // give me the template that fired it
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const T& value)
    {
        static_assert(false, "No default implementation of JsonSerializer<T>::write");
        throw std::runtime_error(std::format("No implementation of JsonSerializer<{}>", typeid(T).name()));
        // static_assert(false, "No Implementation of JsonSerializer<T>::write for this type!");
    }
};

template<>
struct JsonSerializer<bool>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, bool& value)
    {
        if(s.isBool())
        {
            value = s.asBool();
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting bool, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const bool& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<uint8_t>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, uint8_t& value)
    {
        if(s.isUInt())
        {
            value = static_cast<uint8_t>(s.asInt());
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting uint8, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const uint8_t& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<uint16_t>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, uint16_t& value)
    {
        if(s.isUInt())
        {
            value = static_cast<uint16_t>(s.asInt());
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting uint16, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const uint16_t& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<int16_t>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, int16_t& value)
    {
        if(s.isInt())
        {
            value = static_cast<int16_t>(s.asInt());
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting int16, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const uint16_t& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<int32_t>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, int32_t& value)
    {
        if(s.isInt())
        {
            value = s.asInt();
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting int32, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const int32_t& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<int64_t>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, int64_t& value)
    {
        if(s.isInt64())
        {
            value = s.asInt64();
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting int64, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const int64_t& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<uint32_t>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, uint32_t& value)
    {
        if(s.isUInt())
        {
            value = s.asUInt();
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting uint32, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const uint32_t& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<uint64_t>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, uint64_t& value)
    {
        if(s.isUInt64())
        {
            value = s.asUInt64();
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting uint64, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const uint64_t& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<float>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, float& value)
    {
        if(s.isDouble())
        {
            value = s.asDouble();
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting float, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const float& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<double>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, double& value)
    {
        if(s.isDouble())
        {
            value = s.asDouble();
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting double, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const double& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<>
struct JsonSerializer<std::string>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, std::string& value)
    {
        if(s.isString())
        {
            value = s.asString();
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting string, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const std::string& value)
    {
        s = value;
        return Ok<void>();
    };
};

template<class T>
struct JsonSerializer<std::vector<T>>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, std::vector<T>& value)
    {
        if(s.isArray())
        {
            value.clear();
            value.reserve(s.size());
            for(auto& itemJson : value)
            {
                T itemValue;
                CHECK_RESULT(JsonSerializer<T>::read(itemJson, itemValue));
                value.push_back(std::move(itemValue));
            }
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting array, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const std::vector<T>& value)
    {
        s = Json::Value();
        for(auto& item : value)
        {
            Json::Value itemJson;
            CHECK_RESULT(JsonSerializer<T>::write(itemJson, item));
            s.append(itemJson);
        }
        return Ok<void>();
    };
};

template<class T>
struct JsonSerializer<std::unordered_map<std::string, T>>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, std::unordered_map<std::string, T>& value)
    {
        if(s.isObject())
        {
            value.clear();
            value.reserve(s.size());
            for(auto& key : s.getMemberNames())
            {
                T itemValue;
                CHECK_RESULT(JsonSerializer<T>::read(s[key], itemValue));
                value.push_back(std::move(itemValue));
            }
            return Ok<void>();
        }
        return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                       std::format("expecting object, value was {}", s.toStyledString())));
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, std::unordered_map<std::string, T>& value)
    {
        s = Json::Value();
        for(auto& pair : value)
        {
            Json::Value itemJson;
            CHECK_RESULT(JsonSerializer<T>::write(itemJson, pair.second));
            s[pair.first] = itemJson;
        }
        return Ok<void>();
    };
};

template<glm::length_t L, typename T, glm::qualifier Q>
struct JsonSerializer<glm::vec<L, T, Q>>
{
    static Result<void, JsonSerializerError> read(const Json::Value& json, glm::vec<L, T, Q>& value)
    {
        if(!json.isArray())
            return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                           std::format("expecting object, value was {}", json.toStyledString())));

        size_t i = 0;
        for(auto& entry : json)
        {
            T data;
            CHECK_RESULT(JsonSerializer<T>::read(entry, data));
            if(i >= L)
                return Err(JsonSerializerError(
                    JsonSerializerError::WrongType,
                    std::format("expected array of size {}, value was {}", L, json.toStyledString())));
            value[i++] = data;
        }

        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const glm::vec<L, T, Q>& value)
    {
        json = Json::Value();
        for(size_t i = 0; i < L; ++i)
        {
            Json::Value entry;
            CHECK_RESULT(JsonSerializer<T>::write(entry, value[i]));
            json.append(entry);
        }

        return Ok<void>();
    };
};

template<class T, size_t C, size_t R, glm::qualifier Q>
struct JsonSerializer<glm::mat<C, R, T, Q>>
{
    static Result<void, JsonSerializerError> read(const Json::Value& json, glm::mat<C, R, T, Q>& value)
    {
        if(!json.isArray())
            return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                           std::format("expecting object, value was {}", json.toStyledString())));

        size_t i = 0;
        for(auto& entry : json)
        {
            T data;
            CHECK_RESULT(JsonSerializer<T>::read(entry, data));
            if(i >= C * R)
                return Err(JsonSerializerError(
                    JsonSerializerError::WrongType,
                    std::format("expected array of size {}, value was {}", C * R, json.toStyledString())));
            value[i++] = data;
        }

        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const glm::mat<C, R, T, Q>& value)
    {
        json = Json::Value();
        for(size_t i = 0; i < C * R; ++i)
        {
            Json::Value entry;
            CHECK_RESULT(JsonSerializer<T>::write(entry, value[i]));
            json.append(entry);
        }

        return Ok<void>();
    };
};

template<class... Args>
struct JsonSerializer<std::variant<Args...>>
{

    template<size_t ArgIndex, class T, class... Remaining>
    static Result<std::variant<Args...>, JsonSerializerError> readT(const Json::Value& json, size_t typeIndex)
    {
        if(typeIndex == ArgIndex)
        {
            T value;
            auto res = JsonSerializer<T>::read(json, value);
            if(!res)
                return Err(res.err());
            return Ok(std::variant<Args...>(std::move(value)));
        }

        if constexpr(sizeof...(Remaining))
            return readT<ArgIndex + 1, Remaining...>(json, typeIndex);
        return Err(JsonSerializerError(JsonSerializerError::WrongStringFormat,
                                       std::format("No variant type matches {}", typeIndex)));
    }

    template<size_t ArgIndex, class T, class... Remaining>
    static Result<void, JsonSerializerError>
    writeT(Json::Value& json, size_t typeIndex, const std::variant<Args...>& value)
    {
        if(typeIndex == ArgIndex)
            return JsonSerializer<T>::write(json, std::get<ArgIndex>(value));

        if constexpr(sizeof...(Remaining))
            return writeT<ArgIndex + 1, Remaining...>(json, typeIndex, value);
        return Err(JsonSerializerError(JsonSerializerError::ParserError, "Unreachable variant serializer error"));
    }

    static Result<void, JsonSerializerError> read(const Json::Value& json, std::variant<Args...>& value)
    {
        if(!json.isObject())
            return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                           std::format("expecting object, value was {}", json.toStyledString())));

        if(!json.isMember("Type") || !json.isMember("Value"))
            return Err(
                JsonSerializerError(JsonSerializerError::WrongType,
                                    std::format("expecting variant object, with Type and Value keys, instead found: {}",
                                                json.toStyledString())));
        auto res = readT<0, Args...>(json["Value"], json["Type"].asInt());
        CHECK_RESULT(res);
        value = std::move(res.ok());
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const std::variant<Args...>& value)
    {
        json = Json::Value();
        size_t index = value.index();
        json["Type"] = index;
        return writeT<0, Args...>(json["Value"], index, value);
    };
};

struct JsonParseUtil

{
    template<class T>
    static Result<void, JsonSerializerError> read(const Json::Value& json, T& value)
    {
        return JsonSerializer<T>::read(json, value);
    }

    template<class T>
    static Result<void, JsonSerializerError> write(Json::Value& json, const T& value)
    {
        return JsonSerializer<T>::write(json, value);
    }
};

#define X_SERIALIZE_JSON_MEMBER_READ(type, name) CHECK_RESULT(JsonParseUtil::read(json[#name], value.name))
#define X_SERIALIZE_JSON_MEMBER_WRITE(type, name) CHECK_RESULT(JsonParseUtil::write(json[#name], value.name))

#define DEF_JSON_SERIALIZER(type, serialize, deserialize)                                                              \
    struct JsonSerializer<type>                                                                                        \
    {                                                                                                                  \
        static Result<void, JsonSerializerError> read(const Json::Value& json, type& value)                            \
        {                                                                                                              \
            serialize;                                                                                                 \
            return Ok<void>();                                                                                         \
        }                                                                                                              \
                                                                                                                       \
        static Result<void, JsonSerializerError> write(Json::Value& json, const type& value)                           \
        {                                                                                                              \
            deserialize;                                                                                               \
            return Ok<void>();                                                                                         \
        }                                                                                                              \
    };
