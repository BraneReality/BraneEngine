
#ifndef H_BRANE_MUTEX
#define H_BRANE_MUTEX

#include <mutex>
#include "option.h"
#include <shared_mutex>

template<class T>
class Mutex
{
    struct Inner
    {
        T value;
        mutable std::mutex m;

        Inner() = default;

        Inner(T value) : value(std::move(value)) {}
    };

    // We have this in a pointer so the base class is movable
    std::unique_ptr<Inner> _inner;

  public:
    Mutex()
    {
        _inner = std::make_unique<Inner>();
    }

    template<class... Args>
    Mutex(Args... args)
    {
        _inner = std::make_unique<Inner>(T(args...));
    }

    class Lock
    {
        std::lock_guard<std::mutex> lock;
        T* value;


      public:
        Lock(T* value, std::mutex& m) : value(value), lock(m, std::adopt_lock) {}

        T* operator->()
        {
            return value;
        }

        T& operator*()
        {
            return *value;
        }

        const T* operator->() const
        {
            return value;
        }

        const T& operator*() const
        {
            return *value;
        }
    };

    class ConstLock
    {
        std::lock_guard<std::mutex> lock;
        const T* value;


      public:
        ConstLock(const T* value, std::mutex& m) : value(value), lock(m, std::adopt_lock) {}

        const T* operator->() const
        {
            return value;
        }

        const T& operator*() const
        {
            return *value;
        }
    };

    Lock lock()
    {
        _inner->m.lock();
        return Lock(&_inner->value, _inner->m);
    }

    ConstLock lock() const
    {
        _inner->m.lock();
        return ConstLock(&_inner->value, _inner->m);
    }

    Option<Lock> try_lock()
    {
        if(_inner->m.try_lock())
            return Some(Lock(&_inner->value, _inner->m));
        else
            return None();
    }

    Option<const Lock> try_lock() const
    {
        if(_inner->m.try_lock())
            return Some(Lock(&_inner->value, _inner->m));
        else
            return None();
    }
};

template<class T>
class RwMutex
{

    struct Inner
    {
        T value;
        mutable std::shared_mutex m;

        Inner() = default;

        Inner(T value) : value(std::move(value)) {}
    };

    std::unique_ptr<Inner> _inner;

  public:
    RwMutex()
    {
        _inner = std::make_unique<Inner>();
    }

    template<class... Args>
    RwMutex(Args... args)
    {
        _inner = std::make_unique<Inner>(T(args...));
    }

    class Lock
    {
        std::unique_lock<std::shared_mutex> lock;
        T* value;

      public:
        Lock(T* value, std::shared_mutex& m) : value(value), lock(m, std::adopt_lock) {}

        T* operator->()
        {
            return value;
        }

        T& operator*()
        {
            return *value;
        }

        const T* operator->() const
        {
            return value;
        }

        const T& operator*() const
        {
            return *value;
        }
    };

    class SharedLock
    {
        std::shared_lock<std::shared_mutex> lock;
        const T* value;

      public:
        SharedLock(const T* value, std::shared_mutex& m) : value(value), lock(m, std::adopt_lock) {}

        const T* operator->() const
        {
            return value;
        }

        const T& operator*() const
        {
            return *value;
        }
    };

    Lock lock()
    {
        _inner->m.lock();
        return Lock(&_inner->value, _inner->m);
    }

    const Lock lock() const
    {
        _inner->m.lock();
        return Lock(&_inner->value, _inner->m);
    }

    Option<Lock> try_lock()
    {
        if(_inner->m.try_lock())
            return Some(Lock(&_inner->value, _inner->m));
        else
            return None();
    }

    Option<const Lock> try_lock() const
    {
        if(_inner->m.try_lock())
            return Some(Lock(&_inner->value, _inner->m));
        else
            return None();
    }

    const SharedLock lockShared() const
    {
        _inner->m.lock_shared();
        return SharedLock(&_inner->value, _inner->m);
    }

    Option<const SharedLock> try_lock_shared() const
    {
        if(_inner->m.try_lock_shared())
            return Some<const SharedLock>(SharedLock(&_inner->value, _inner->m));
        else
            return None();
    }
};

#endif
