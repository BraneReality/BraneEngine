
#ifndef H_BRANE_MUTEX
#define H_BRANE_MUTEX

#include <mutex>
#include "option.h"

template<class T>
class Mutex
{
    T value;
    mutable std::mutex m;

  public:
    class Lock
    {
        std::lock_guard<std::mutex> lock;
        T* value;

        Lock(T* value, std::mutex& m) : value(value), lock(m, std::adopt_lock) {}

        T& operator->()
        {
            return *value;
        }

        T& operator*()
        {
            return *value;
        }

        const T& operator->() const
        {
            return *value;
        }

        const T& operator*() const
        {
            return *value;
        }
    };

    Lock lock()
    {
        m.lock();
        return Lock(&value, m);
    }

    const Lock lock() const
    {
        m.lock();
        return Lock(&value, m);
    }

    Option<Lock> try_lock()
    {
        if(m.try_lock())
            return Some(Lock(&value, m));
        else
            return None();
    }

    Option<const Lock> try_lock() const
    {
        if(m.try_lock())
            return Some(Lock(&value, m));
        else
            return None();
    }
};

#endif
