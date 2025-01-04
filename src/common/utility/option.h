#pragma once
#include <functional>
#include <variant>
#include "serializedData.h"

struct None
{
};

template<class T>
struct Some
{
    T value;

    Some(T val)
        : value([](T val) -> T {
              if constexpr(std::is_reference<T>())
                  return val;
              else
                  return std::move(val);
          }(val))
    {}
};

template<class V>
class Option
{
    using Value = std::variant<None, Some<V>>;
    Value _value;

  public:
    Option()
    {
        _value = None();
    }

    Option(Value value) : _value(std::move(value)) {}

    Option(None none) : Option(Value(none)) {};
    Option(Some<V> some) : Option(Value(some)) {};

    bool isSome() const
    {
        return std::holds_alternative<Some<V>>(_value);
    }

    bool isNone() const
    {
        return std::holds_alternative<None>(_value);
    }

    operator bool() const
    {
        return isSome();
    }

    Option<V>& operator=(Some<V> s)
    {
        _value = s;
        return *this;
    }

    Option<V>& operator=(None n)
    {
        _value = n;
        return *this;
    }

    V& value()
    {
        return std::get<Some<V>>(_value).value;
    }

    const V& value() const
    {
        return std::get<Some<V>>(_value).value;
    }

    template<class T>
    Option<T> map(std::function<T(V)> f)
    {
        return MATCHV(std::move(_value), [&](Some<V> value) {
            return Option<T>(Some<T>(f(std::move(value.value))));
        }, [](None none) { return Option(none); });
    }

    V valueOrDefault(V defaultValue)
    {
        return MATCHV(std::move(_value), [&](Some<V> value) {
            return std::move(value.value);
        }, [defaultValue](None none) { return std::forward(defaultValue); });
    }
};

template<class T>
struct Serializer<Option<T>>
{
    static Result<void, SerializerError> read(InputSerializer& s, Option<T>& value)
    {
        bool hasValue;
        s >> hasValue;
        if(hasValue)
        {
            T data;
            auto res = Serializer<T>::read(s, data);
            if(!res)
                return res;
            value = Some(data);
        }
        return Ok();
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const Option<T>& value)
    {
        bool hasValue = value;
        s << hasValue;
        if(hasValue)
            return Serializer<T>::write(s, value.value());
        return Ok();
    }
};
