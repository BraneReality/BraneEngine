#pragma once

#include <memory>
#include "jsonSerializer.h"

/// shared_ptr wrapper that must be initialized
template<class T>
class SPtr
{
    std::shared_ptr<T> _value;

  public:
    template<class... Args>
    SPtr<T>(Args... args)
    {
        _value = std::make_shared<T>(args...);
    }

    SPtr<T>(std::shared_ptr<T> notNull)
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
};

template<class T>
struct JsonSerializer<SPtr<T>>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, SPtr<T>& value)
    {
        return JsonSerializer<T>::read(s, *value.get());
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const SPtr<T>& value)
    {
        return JsonSerializer<T>::write(s, *value.get());
    };
};
