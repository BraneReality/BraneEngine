#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <typeinfo>
#include <variant>
#include <vector>
#include "json/json.h"
#include "utility/assert.h"
#include "utility/result.h"

class SerializationError : virtual public std::runtime_error
{
  public:
    explicit SerializationError(const std::type_info& t)
        : std::runtime_error(std::string("type: ") + t.name() + " could not be serialized")
    {}
};

struct SerializerError
{
    enum Type
    {
        NotEnoughData,
        WrongFormat
    } type;

    inline SerializerError(Type type) : type(type) {}

    std::string_view toString() const;
};

class InputSerializer;
class OutputSerializer;

template<class T>
struct Serializer
{
    static Result<void, SerializerError> read(InputSerializer& s, T& value);

    static Result<void, SerializerError> write(OutputSerializer& s, const T& value);
};

class SerializedData
{
    std::vector<uint8_t> _data;

  public:
    SerializedData() = default;

    SerializedData(const SerializedData& s) = delete;

    SerializedData(SerializedData&& s)
    {
        _data = std::move(s._data);
    }

    inline uint8_t& operator[](size_t index)
    {
        return _data[index];
    }

    inline const uint8_t& operator[](size_t index) const
    {
        return _data[index];
    }

    inline void clear()
    {
        _data.clear();
    }

    inline void resize(size_t newSize)
    {
        _data.resize(newSize);
    }

    inline size_t size() const
    {
        return _data.size();
    }

    inline uint8_t* data()
    {
        return _data.data();
    }

    inline const uint8_t* data() const
    {
        return _data.data();
    }

    inline std::vector<uint8_t>& vector()
    {
        return _data;
    }
};

class InputSerializer
{
    struct Context
    {
        size_t index = 0;
        const SerializedData& data;
    };

    std::shared_ptr<Context> _ctx = nullptr;

  public:
    InputSerializer(const SerializedData& data)
    {
        _ctx = std::make_shared<Context>(0, data);
    }

    const SerializedData& data() const
    {
        return _ctx->data;
    }

    friend std::ostream& operator<<(std::ostream& os, const InputSerializer& s)
    {
        os << " Serialized Data: ";
        for(int i = 0; i < s._ctx->data.size(); ++i)
        {
            if(i % 80 == 0)
                std::cout << "\n";
            std::cout << s._ctx->data[i];
        }
        std::cout << std::endl;
        return os;
    }

    // Read a raw bytes from the buffer into the provide array
    Result<void, SerializerError> read(void* value, size_t count)
    {
        if(_ctx->index + count > _ctx->data.size())
            return Err(SerializerError(SerializerError::NotEnoughData));

        std::memcpy(value, (void*)(_ctx->data.data() + _ctx->index), count);

        _ctx->index += count;
        return Ok();
    }

    template<typename T>
    Result<T, SerializerError> peek()
    {
        static_assert(std::is_trivially_copyable<T>());
        if(_ctx->index + sizeof(T) > _ctx->data.size())
            return Err(SerializerError(SerializerError::NotEnoughData));
        size_t index = _ctx->index;
        T o;
        *this >> o;
        _ctx->index = index;
        return Ok(o);
    }

    size_t getPos() const
    {
        return _ctx->index;
    }

    void setPos(size_t index)
    {
        assert(index <= _ctx->data.size());
        _ctx->index = index;
    }

    bool isDone() const
    {
        return _ctx->index == _ctx->data.size();
    }
};

class OutputSerializer
{
    SerializedData& _data;

  public:
    OutputSerializer(SerializedData& data) : _data(data) {};

    const SerializedData& data() const
    {
        return _data;
    }

    Result<void, SerializerError> write(const void* src, size_t size)
    {
        size_t index = _data.size();
        _data.resize(index + size);
        std::memcpy(&_data[index], src, size);
        return Ok();
    }

    Result<void, SerializerError> replace(size_t pos, const void* src, size_t size)
    {
        if(pos + size >= _data.size())
            return Err(SerializerError(SerializerError::NotEnoughData));
        std::memcpy(&_data[pos], src, size);
        return Ok();
    }

    size_t size() const
    {
        return _data.size();
    }
};

template<class T>
Result<void, SerializerError> Serializer<T>::read(InputSerializer& s, T& value)
{
    static_assert(std::is_trivially_copyable<T>(),
                  "Default serializer is only implemented for trivially copyable types, consider creating a custom "
                  "implementation of Serializer<T> to support this type");
    return s.read(&value, sizeof(T));
}

template<class T>
Result<void, SerializerError> Serializer<T>::write(OutputSerializer& s, const T& value)
{
    static_assert(std::is_trivially_copyable<T>(),
                  "Default serializer is only implemented for trivially copyable types, consider creating a custom "
                  "implementation of Serializer<T> to support this type");
    return s.write(&value, sizeof(value));
}

template<>
struct Serializer<std::string>
{
    static Result<void, SerializerError> read(InputSerializer& s, std::string& value)
    {
        uint32_t count;
        auto res = Serializer<uint32_t>::read(s, count);
        if(!res)
            return res;
        value.resize(count);
        if(count == 0)
            return Ok();
        return s.read(value.data(), count);
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const std::string& value)
    {
        auto count = static_cast<uint32_t>(value.size());
        auto res = Serializer<uint32_t>::write(s, count);
        if(!res)
            return res;
        if(count == 0)
            return Ok();
        return s.write(value.data(), count);
    }
};

template<class T>
struct Serializer<std::vector<T>>
{
    static Result<void, SerializerError> read(InputSerializer& s, std::vector<T>& value)
    {
        uint32_t count;
        auto res = Serializer<uint32_t>::read(s, count);
        if(!res)
            return res;
        value.resize(count);
        if(count == 0)
            return Ok();
        if constexpr(std::is_trivially_copyable<T>())
            return s.read(value.data(), sizeof(T) * count);

        for(auto& e : value)
        {
            auto res = Serializer<T>::read(s, e);
            if(!res)
                return res;
        }

        return Ok();
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const std::vector<T>& value)
    {
        auto count = static_cast<uint32_t>(value.size());
        auto res = Serializer<uint32_t>::write(s, count);
        if(!res)
            return res;
        if(count == 0)
            return Ok();
        if constexpr(std::is_trivially_copyable<T>())
            return s.write(value.data(), count * sizeof(T));

        for(const auto& e : value)
        {
            auto res = Serializer<T>::write(s, e);
            if(!res)
                return res;
        }
        return Ok();
    }
};

template<>
struct Serializer<Json::Value>
{
    static Result<void, SerializerError> read(InputSerializer& s, Json::Value& value)
    {
        std::string jsonString;
        auto res = Serializer<std::string>::read(s, jsonString);
        if(!res)
            return Err(res.err());

        Json::CharReaderBuilder builder;
        Json::CharReader* reader = builder.newCharReader();

        std::string errors;
        bool success = reader->parse(jsonString.c_str(), jsonString.c_str() + jsonString.size(), &value, &errors);
        delete reader;
        if(!success)
            return Err(SerializerError(SerializerError::WrongFormat));

        return Ok();
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const Json::Value& value)
    {
        Json::FastWriter writer;
        std::string jsonString = writer.write(value);

        return Serializer<std::string>::write(s, jsonString);
    }
};

template<typename T>
Result<InputSerializer&, SerializerError> operator>>(Result<InputSerializer&, SerializerError> r, T& data)
{
    if(r)
    {
        InputSerializer& s = r.ok();
        s >> data;
        return Ok<InputSerializer&>(s);
    }
    return r;
}

template<typename T>
Result<OutputSerializer&, SerializerError> operator<<(Result<OutputSerializer&, SerializerError> r, const T& data)
{
    if(r)
    {
        OutputSerializer& s = r.ok();
        s << data;
        return Ok<OutputSerializer&>(s);
    }
    return r;
}

template<class... Args>
struct Serializer<std::variant<Args...>>
{
    static constexpr size_t indexCount = std::variant_size_v<std::variant<Args...>>;
    using IndexType = std::conditional_t<(indexCount < 256), uint8_t, uint16_t>;

    template<size_t ArgIndex, class T, class... Remaining>
    static Result<std::variant<Args...>, SerializerError> readT(InputSerializer& s, size_t typeIndex)
    {
        if(typeIndex == ArgIndex)
        {
            T value;
            auto res = Serializer<T>::read(s, value);
            if(!res)
                return Err(res.err());
            return Ok(std::variant<Args...>(std::move(value)));
        }

        if constexpr(sizeof...(Remaining))
            return readT<ArgIndex + 1, Remaining...>(s, typeIndex);
        return Err(SerializerError(SerializerError::WrongFormat));
    }

    template<size_t ArgIndex, class T, class... Remaining>
    static Result<void, SerializerError>
    writeT(OutputSerializer& s, size_t typeIndex, const std::variant<Args...>& value)
    {
        if(typeIndex == ArgIndex)
            return Serializer<T>::write(s, std::get<ArgIndex>(value));

        if constexpr(sizeof...(Remaining))
            return writeT<ArgIndex + 1, Remaining...>(s, typeIndex, value);
        return Err(SerializerError(SerializerError::WrongFormat));
    }

    static Result<void, SerializerError> read(InputSerializer& s, std::variant<Args...>& value)
    {
        IndexType index;
        s >> index;
        if(index >= indexCount)
            return Err(SerializerError(SerializerError::WrongFormat));
        auto res = readT<0, Args...>(s, index);
        if(!res)
            return Err(res.err());
        value = res.ok();
        return Ok();
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const std::variant<Args...>& value)
    {
        IndexType index = value.index();
        s << index;
        return writeT<0, Args...>(s, index, value);
    }
};

template<typename T>
Result<InputSerializer&, SerializerError> operator>>(InputSerializer& s, T& data)
{
    return Serializer<T>::read(s, data).template map<InputSerializer&>([&]() -> InputSerializer& { return s; });
}

template<typename T>
Result<OutputSerializer&, SerializerError> operator<<(OutputSerializer& s, const T& data)
{
    return Serializer<T>::write(s, data).template map<OutputSerializer&>([&]() -> OutputSerializer& { return s; });
}
