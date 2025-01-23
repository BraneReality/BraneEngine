//
// Created by eli on 6/29/2022.
//

#ifndef BRANEENGINE_INLINEARRAY_H
#define BRANEENGINE_INLINEARRAY_H

#include <cassert>
#include <cstdint>
#include <vector>
#include "serializedData.h"

template<typename T, size_t Count>
class InlineArray
{
    T _localData[Count];
    std::vector<T>* _externalData = nullptr;
    size_t _size = 0;

  public:
    InlineArray() = default;

    InlineArray(const InlineArray& o) noexcept
    {
        for(size_t i = 0; i < Count; ++i)
        {
            _localData[i] = o._localData[i];
        }
        _size = o._size;
        _externalData = nullptr;
        if(o._externalData)
        {
            _externalData = new std::vector<T>(o._externalData->size());
            std::copy(o._externalData->begin(), o._externalData->end(), _externalData->begin());
        }
    }

    void operator=(const InlineArray& o) noexcept
    {
        for(size_t i = 0; i < Count; ++i)
        {
            _localData[i] = o._localData[i];
        }
        _size = o._size;
        _externalData = nullptr;
        if(o._externalData)
        {
            _externalData = new std::vector<T>(o._externalData->size());
            std::copy(o._externalData->begin(), o._externalData->end(), _externalData->begin());
        }
    }

    InlineArray(InlineArray&& o) noexcept
    {
        for(size_t i = 0; i < Count; ++i)
        {
            _localData[i] = std::move(o._localData[i]);
        }
        _externalData = o._externalData;
        _size = o._size;
        o._externalData = nullptr;
        o._size = 0;
    }

    void operator=(InlineArray&& o) noexcept
    {
        for(size_t i = 0; i < Count; ++i)
        {
            _localData[i] = std::move(o._localData[i]);
        }
        _externalData = o._externalData;
        o._externalData = nullptr;
        _size = o._size;
    }

    ~InlineArray()
    {
        if(_externalData)
            delete _externalData;
    }

    T& operator[](size_t index)
    {
        assert(index < _size);
        if(index < Count)
            return _localData[index];

        assert(_externalData);
        index -= Count;
        return (*_externalData)[index];
    }

    const T& operator[](size_t index) const
    {
        assert(index < _size);
        if(index < Count)
            return _localData[index];

        assert(_externalData);
        index -= Count;
        return (*_externalData)[index];
    }

    void push_back(const T& item)
    {
        if(_size < Count)
        {
            _localData[_size++] = item;
            return;
        }

        if(!_externalData)
            _externalData = new std::vector<T>();
        _externalData->push_back(item);
        ++_size;
    }

    void push_back(T&& item)
    {
        if(_size < Count)
        {
            _localData[_size++] = item;
            return;
        }

        if(!_externalData)
            _externalData = new std::vector<T>();
        _externalData->push_back(item);
        ++_size;
    }

    void erase(size_t index)
    {
        assert(index < _size);
        for(size_t i = index; i < _size - 1; i++)
        {
            (*this)[i] = (*this)[i + 1];
        }
        if(index >= Count)
            _externalData->resize(_externalData->size() - 1);
        --_size;
    }

    size_t size() const
    {
        return _size;
    }

    class iterator
    {
        InlineArray<T, Count>& _ref;
        size_t _index;

      public:
        iterator(InlineArray<T, Count>& ref, size_t index) : _ref(ref), _index(index) {};

        void operator++()
        {
            ++_index;
        }

        void operator+(size_t index)
        {
            _index += index;
        }

        bool operator!=(const iterator& o) const
        {
            return _index != o._index;
        }

        bool operator==(const iterator& o) const
        {
            return _index == o._index;
        }

        T& operator*()
        {
            return _ref[_index];
        };

        using iterator_category = std::random_access_iterator_tag;
        using reference = T&;
        using pointer = T*;
    };

    class const_iterator
    {
        const InlineArray<T, Count>& _ref;
        size_t _index;

      public:
        const_iterator(const InlineArray<T, Count>& ref, size_t index) : _ref(ref), _index(index) {};

        void operator++()
        {
            ++_index;
        }

        void operator+(size_t index)
        {
            _index += index;
        }

        bool operator!=(const const_iterator& o) const
        {
            return _index != o._index;
        }

        bool operator==(const const_iterator& o) const
        {
            return _index == o._index;
        }

        const T& operator*() const
        {
            return _ref[_index];
        };

        using iterator_category = std::random_access_iterator_tag;
        using reference = T&;
        using pointer = T*;
    };

    iterator begin()
    {
        return iterator(*this, 0);
    }

    iterator end()
    {
        return iterator(*this, _size);
    }

    const_iterator begin() const
    {
        return const_iterator(*this, 0);
    }

    const_iterator end() const
    {
        return const_iterator(*this, _size);
    }
};

using inlineUIntArray = InlineArray<uint32_t, 4>;
using inlineIntArray = InlineArray<int32_t, 4>;
using inlineFloatArray = InlineArray<float, 4>;
struct EntityID;
using inlineEntityIDArray = InlineArray<EntityID, 4>;

template<class T, size_t C>
struct Serializer<InlineArray<T, C>>
{
    static Result<void, SerializerError> read(InputSerializer& s, InlineArray<T, C>& value)
    {
        uint32_t count;
        auto res = Serializer<uint32_t>::read(s, count);
        if(!res)
            return res;
        if(count == 0)
            return Ok<void>();
        while(value.size() < count)
            value.push_back(T());

        for(auto& e : value)
        {
            auto res = Serializer<T>::read(s, e);
            if(!res)
                return res;
        }

        return Ok<void>();
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const InlineArray<T, C>& value)
    {
        auto count = static_cast<uint32_t>(value.size());
        auto res = Serializer<uint32_t>::write(s, count);
        if(!res)
            return res;
        if(count == 0)
            return Ok<void>();

        for(const auto& e : value)
        {
            auto res = Serializer<T>::write(s, e);
            if(!res)
                return res;
        }
        return Ok<void>();
    }
};

#endif // BRANEENGINE_INLINEARRAY_H
