
#ifndef H_BRANE_MUTEX
#define H_BRANE_MUTEX

#include <mutex>
#include <optional>
#include <thread>

template<class T>
class Mutex
{
    T value;
    std::mutex m;

    class Lock
    {
        std::lock_guard<std::mutex> lock;
        T& value;

        Lock(T& value, std::mutex& m) : value(value), lock(m, std::adopt_lock) {}

        T& operator->()
        {
            return value;
        }

        T& operator*()
        {
            return value;
        }
    };

    Lock lock()
    {
        m.lock();
        return Lock(value, m);
    }

    std::optional<Lock> try_lock()
    {
        if(m.try_lock())
            return Lock(value, m);
        else
            return std::nullopt;
    }
};

#endif
