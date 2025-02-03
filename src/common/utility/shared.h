#pragma once

#include <cassert>
#include <memory>
#include "jsonSerializer.h"

/// shared_ptr wrapper that must be initialized
template<class T>
class Shared
{
    std::shared_ptr<T> _value;

  public:
    Shared<T>(T* notNull)
    {
        assert(notNull);
        _value = std::shared_ptr<T>(notNull);
    }

    Shared<T>(std::shared_ptr<T> notNull)
    {
        assert(notNull);
        _value = notNull;
    }

    T* get()
    {
        return _value.get();
    }

    const T* get() const
    {
        return _value.get();
    }

    T* operator->()
    {
        return _value.get();
    }

    const T* operator->() const
    {
        return _value.get();
    }

    T& operator&()
    {
        return *_value;
    }

    const T& operator&() const
    {
        return *_value;
    }

    operator std::shared_ptr<T>() const
    {
        return _value;
    }

    Shared<T>& operator=(std::shared_ptr<T> newValue)
    {
        _value = newValue;
        return *this;
    }
};

template<class T>
struct JsonSerializer<Shared<T>>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, Shared<T>& value)
    {
        return JsonSerializer<T>::read(s, *value.get());
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const Shared<T>& value)
    {
        return JsonSerializer<T>::write(s, *value.get());
    };
};
