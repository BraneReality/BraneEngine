
#ifndef H_BRANE_RESULT
#define H_BRANE_RESULT

#include <functional>
#include <string>
#include <variant>
#include "assert.h"
#include "variantMatch.h"

template<typename T = void>
struct Ok
{
    T value;

    Ok() = default;

    Ok(T val)
        : value([](T val) -> T {
              if constexpr(std::is_reference<T>())
                  return val;
              else
                  return std::move(val);
          }(val))
    {}
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
    using Value = std::variant<Ok<V>, Err<E>>;

    Value _value;

  public:
    Result(Value value) : _value(std::move(value)) {}

    Result(Ok<V> ok) : Result(Value(ok)) {};
    Result(Err<E> err) : Result(Value(err)) {};

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
        return MATCHV(std::move(_value), [](Ok<V> ok) -> V {
            if constexpr(std::is_reference<V>() || std::is_pointer<V>())
                return ok.value;
            else
                return std::move(ok.value);
        }, [](Err<E> err) -> V { throw std::runtime_error("ok() called on result that contained an err value!"); });
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
