
#ifndef H_BRANE_RESULT
#define H_BRANE_RESULT

#include <functional>
#include <stdexcept>
#include <string>
#include <variant>
#include "variantMatch.h"
#include <type_traits>

#define CHECK_RESULT(result)                                                                                           \
    if(result.isErr())                                                                                                 \
        return Err(result.err());

template<class T = void>
struct Ok
{
    static_assert(std::negation<std::is_reference<T>>(), "Results cannot contain references");
    T value;

    Ok(T val) : value(std::move(val)) {}
};

// Specialization for void
template<>
struct Ok<void>
{
};

template<class T>
struct Err
{
    T data;
    Err(T data) : data(std::move(data)) {};
};

template<class V = void, class E = std::string>
class Result
{
  public:
    using Value = std::variant<Ok<V>, Err<E>>;

  private:
    Value _value;

  public:
    Result(Value&& value) : _value(std::forward<Value>(value)) {};
    Result(Ok<V>&& ok) : Result(Value(std::forward<Ok<V>>(ok))) {};
    Result(Err<E>&& err) : Result(Value(std::forward<Err<E>>(err))) {};

    Result(Result&&) = default;
    Result(const Result&) = default;

    ~Result()
    {
        // TODO consider if we should force result handling
    }

    bool isOk() const
    {
        return std::holds_alternative<Ok<V>>(_value);
    }

    bool isErr() const
    {
        return std::holds_alternative<Err<E>>(_value);
    }

    operator bool() const
    {
        return isOk();
    }

    /// consumes the result
    V ok()
        requires std::negation_v<std::is_void<V>>
    {
        if(std::holds_alternative<Ok<V>>(_value))
            if constexpr(std::is_reference<V>() || std::is_pointer<V>())
                return std::get<Ok<V>>(_value).value;
            else
                return std::move(std::get<Ok<V>>(_value).value);
        else
            throw std::runtime_error("ok() called on result that contained an err value!");
    };

    /// consumes the result
    E err()
    {
        return MATCHV(std::move(_value), [](Ok<V> ok) -> E {
            throw std::runtime_error("err() called on result that contained an ok value!");
        }, [](Err<E> err) -> E {
            if constexpr(std::is_reference<E>() || std::is_pointer<E>())
                return err.data;
            else
                return std::move(err.data);
        });
    }

    template<class T, class C = V>
    Result<T, E> map(std::function<T(C)> f)
        requires std::negation_v<std::is_void<C>>
    {
        return MATCHV(std::move(_value), [&](Ok<V> value) {
            return Result<T, E>(Ok<T>(f(std::move(value.value))));
        }, [](Err<E> err) { return Result<T, E>(std::move(err)); });
    }

    template<class T, class C = V>
    Result<T, E> map(std::function<T()> f)
        requires std::is_void_v<C>
    {
        return MATCHV(std::move(_value), [&](Ok<V> value) { return Result<T, E>(Ok<T>(f())); }, [](Err<E> err) {
            return Result<T, E>(std::move(err));
        });
    }
};

#endif // H_BRANE_RESULT
