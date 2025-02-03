#pragma once
#include <format>
#include <string>
#include <typeinfo>
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

template<class T>
struct JsonSerializer
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, T& value)
    {
        throw std::runtime_error(std::format("No implementation of JsonSerializer<{}>", typeid(T).name()));
        // static_assert(false, "No Implementation of JsonSerializer<T>::read for this type!");
        // commented out static_assert cause it wasn't giving me type names, "it's broken!" cool, how? TELL ME. At least
        // give me the template that fired it
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const T& value)
    {
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
        auto res = JsonSerializer<T>::write(json, value);
        return res;
    }
};
