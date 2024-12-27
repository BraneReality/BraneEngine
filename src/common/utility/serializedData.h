#pragma once

#include <byte.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <regex>
#include <sstream>
#include <typeinfo>
#include <vector>
#include <assets/assetID.h>
#include <json/json.h>
#include <utility/inlineArray.h>

class SerializationError : virtual public std::runtime_error
{
  public:
    explicit SerializationError(const std::type_info& t)
        : std::runtime_error(std::string("type: ") + t.name() + " could not be serialized")
    {}
};

class SerializedData
{
    std::vector<byte> _data;

  public:
    SerializedData() = default;

    SerializedData(const SerializedData& s) = delete;

    SerializedData(SerializedData&& s) { _data = std::move(s._data); }

    inline byte& operator[](size_t index) { return _data[index]; }

    inline const byte& operator[](size_t index) const { return _data[index]; }

    inline void clear() { _data.clear(); }

    inline void resize(size_t newSize) { _data.resize(newSize); }

    inline size_t size() const { return _data.size(); }

    inline byte* data() { return _data.data(); }

    inline const byte* data() const { return _data.data(); }

    inline std::vector<byte>& vector() { return _data; }
};

class InputSerializer
{
    struct Context
    {
        size_t index = 0;
        const SerializedData& data;
        size_t count = 0;
    };

    Context* _ctx = nullptr;

  public:
    InputSerializer(const SerializedData& data) { _ctx = new Context{0, data, 1}; }

    InputSerializer(const InputSerializer& s)
    {
        _ctx = s._ctx;
        ++_ctx->count;
    }

    ~InputSerializer()
    {
        --_ctx->count;
        if(_ctx->count == 0)
            delete _ctx;
    }

    const SerializedData& data() const { return _ctx->data; }

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

    template<typename T>
    friend InputSerializer& operator>>(InputSerializer& s, T& object)
    {
        static_assert(std::is_trivially_copyable<T>());
        if(s._ctx->index + sizeof(T) > s._ctx->data.size())
            throw std::runtime_error("Tried to read past end of serialized data");

        std::memcpy(&object, (void*)(s._ctx->data.data() + s._ctx->index), sizeof(T));

        s._ctx->index += sizeof(T);

        return s;
    }

    template<typename T>
    void readSafeArraySize(T& index) // Call this instead of directly reading sizes to prevent buffer overruns
    {
        *this >> index;
        if(index + _ctx->index > _ctx->data.size())
            throw std::runtime_error("invalid array length in serialized data");
    }

    friend InputSerializer& operator>>(InputSerializer& s, std::vector<std::string>& strings)
    {
        uint32_t numStrings;
        s >> numStrings;
        strings.resize(numStrings);
        for(uint32_t i = 0; i < numStrings; ++i)
        {
            s >> strings[i];
        }
        return s;
    }

    template<typename T>
    friend InputSerializer& operator>>(InputSerializer& s, std::vector<T>& data)
    {
        static_assert(std::is_trivially_copyable<T>());
        uint32_t size;
        s.readSafeArraySize(size);

        data.resize(size / sizeof(T));
        if(size > 0)
            std::memcpy(data.data(), &s._ctx->data[s._ctx->index], size);

        s._ctx->index += size;

        return s;
    }

    template<typename T, size_t Count>
    friend InputSerializer& operator>>(InputSerializer& s, InlineArray<T, Count>& data)
    {
        static_assert(std::is_trivially_copyable<T>());

        uint32_t arrLength;
        s.readSafeArraySize(arrLength);
        // TODO make InlineArray have SerializedData as a friend class and make this more efficient
        for(uint32_t i = 0; i < arrLength; ++i)
        {
            T d;
            s >> d;
            data.push_back(d);
        }

        return s;
    }

    friend InputSerializer& operator>>(InputSerializer& s, std::string& data)
    {
        uint32_t size;
        s.readSafeArraySize(size);

        data.resize(size);
        if(size > 0)
            std::memcpy(data.data(), &s._ctx->data[s._ctx->index], size);

        s._ctx->index += size;

        return s;
    }

    friend InputSerializer& operator>>(InputSerializer& s, std::string_view& data)
    {
        uint32_t size;
        s.readSafeArraySize(size);

        data = std::string_view(reinterpret_cast<const char*>(&s._ctx->data[s._ctx->index]), size);
        s._ctx->index += size;
        return s;
    }

    friend InputSerializer& operator>>(InputSerializer& s, AssetID& id)
    {
        std::string idString;
        s >> idString;
        id = std::move(idString);

        return s;
    }

    friend InputSerializer& operator>>(InputSerializer& s, std::vector<AssetID>& ids)
    {
        uint32_t size;
        s.readSafeArraySize(size);
        ids.resize(size);
        for(uint32_t i = 0; i < size; ++i)
        {
            std::string idString;
            s >> idString;
            ids[i] = std::move(idString);
        }

        return s;
    }

    friend InputSerializer& operator>>(InputSerializer& s, Json::Value& value)
    {
        std::string jsonString;
        s >> jsonString;

        Json::CharReaderBuilder builder;
        Json::CharReader* reader = builder.newCharReader();

        std::string errors;
        bool success = reader->parse(jsonString.c_str(), jsonString.c_str() + jsonString.size(), &value, &errors);
        delete reader;
        if(!success)
            throw std::runtime_error(errors);

        return s;
    }

    void read(void* dest, size_t size)
    {
        if(_ctx->index + size <= _ctx->data.size())
            throw std::runtime_error("Tried to read past end of serialized data");

        std::memcpy(dest, &_ctx->data[_ctx->index], size);
        _ctx->index += size;
    }

    template<typename T>
    T peek()
    {
        static_assert(std::is_trivially_copyable<T>());
        if(_ctx->index + sizeof(T) > _ctx->data.size())
            throw std::runtime_error("Tried to read past end of serialized data");
        size_t index = _ctx->index;
        T o;
        *this >> o;
        _ctx->index = index;
        return o;
    }

    size_t getPos() const { return _ctx->index; }

    void setPos(size_t index)
    {
        assert(index <= _ctx->data.size());
        _ctx->index = index;
    }

    bool isDone() const { return _ctx->index == _ctx->data.size(); }
};

class OutputSerializer
{
    SerializedData& _data;

  public:
    OutputSerializer(SerializedData& data) : _data(data){};

    const SerializedData& data() const { return _data; }

    template<typename T>
    friend OutputSerializer operator<<(OutputSerializer s, const T& data)
    {
        static_assert(std::is_trivially_copyable<T>());

        size_t index = s._data.size();
        s._data.resize(index + sizeof(T));
        std::memcpy(&s._data[index], &data, sizeof(T));

        return s;
    }

    template<typename T>
    friend OutputSerializer operator<<(OutputSerializer s, const std::vector<T>& data)
    {
        static_assert(std::is_trivially_copyable<T>());

        auto arrLength = static_cast<uint32_t>(data.size() * sizeof(T));
        s << arrLength;
        size_t index = s._data.size();
        s._data.resize(index + arrLength);
        if(arrLength > 0)
            std::memcpy(&s._data[index], data.data(), arrLength);

        return s;
    }

    template<typename T, size_t Count>
    friend OutputSerializer operator<<(OutputSerializer s, const InlineArray<T, Count>& data)
    {
        static_assert(std::is_trivially_copyable<T>());

        auto arrLength = static_cast<uint32_t>(data.size());
        s << arrLength;
        // TODO make InlineArray have SerializedData as a friend class and make this more efficient
        for(uint32_t i = 0; i < arrLength; ++i)
        {
            s << data[i];
        }

        return s;
    }

    friend OutputSerializer operator<<(OutputSerializer s, const std::string& data)
    {
        auto arrLength = static_cast<uint32_t>(data.size());
        s << arrLength;
        size_t index = s._data.size();
        s._data.resize(index + arrLength);
        if(arrLength > 0)
            std::memcpy(&s._data[index], data.data(), data.size());

        return s;
    }

    friend OutputSerializer operator<<(OutputSerializer s, const AssetID& id)
    {
        s << id.string();

        return s;
    }

    friend OutputSerializer operator<<(OutputSerializer s, const std::vector<std::string>& strings)
    {
        s << (uint32_t)strings.size();
        for(uint32_t i = 0; i < strings.size(); ++i)
        {
            s << strings[i];
        }
        return s;
    }

    friend OutputSerializer operator<<(OutputSerializer s, const std::vector<AssetID>& ids)
    {
        s << (uint32_t)ids.size();
        for(uint32_t i = 0; i < ids.size(); ++i)
        {
            s << ids[i].string();
        }

        return s;
    }

    friend OutputSerializer& operator<<(OutputSerializer& s, const Json::Value& value)
    {
        Json::FastWriter writer;
        std::string jsonString = writer.write(value);

        s << jsonString;

        return s;
    }

    void write(const void* src, size_t size)
    {
        size_t index = _data.size();
        _data.resize(index + size);
        std::memcpy(&_data[index], src, size);
    }

    void overwrite(size_t pos, const void* src, size_t size)
    {
        if(pos + size >= _data.size())
            throw std::runtime_error("tried to overwrite nonexistent data");
        std::memcpy(&_data[pos], src, size);
    }

    size_t size() const { return _data.size(); }
};
