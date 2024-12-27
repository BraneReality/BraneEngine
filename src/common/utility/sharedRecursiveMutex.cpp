#include "sharedRecursiveMutex.h"
#include <iostream>

SharedRecursiveMutex::SharedRecursiveMutex() {}

void SharedRecursiveMutex::lock()
{
    while(true)
    {
        _m.lock();
        if(_sharedOwners == 0 && (_ownerLockCount == 0 || _owner == std::this_thread::get_id()))
            break;
        _m.unlock();
        std::this_thread::yield();
    }

    _owner = std::this_thread::get_id();
    _ownerLockCount += 1;
    _m.unlock();
}

void SharedRecursiveMutex::unlock()
{
    std::scoped_lock lock(_m);
    assert(_ownerLockCount != 0);
    _ownerLockCount -= 1;
}

void SharedRecursiveMutex::lock_shared()
{
    while(true)
    {
        _m.lock();
        if(_ownerLockCount == 0 || _owner == std::this_thread::get_id())
            break;
        _m.unlock();
        std::this_thread::yield();
    }
    _sharedOwners += 1;
    _m.unlock();
}

void SharedRecursiveMutex::unlock_shared()
{
    std::scoped_lock lock(_m);
    assert(_sharedOwners != 0);
    _sharedOwners -= 1;
}

SharedRecursiveMutex::ScopedLock SharedRecursiveMutex::scopedLock() { return std::move(ScopedLock(*this)); }

SharedRecursiveMutex::SharedScopedLock SharedRecursiveMutex::sharedScopedLock()
{
    return std::move(SharedScopedLock(*this));
}

SharedRecursiveMutex::ScopedLock::ScopedLock(SharedRecursiveMutex& m) : _m(&m) { _m->lock(); }

SharedRecursiveMutex::ScopedLock::ScopedLock(ScopedLock&& o) noexcept
{
    _m = o._m;
    o._m = nullptr;
}

SharedRecursiveMutex::ScopedLock::~ScopedLock()
{
    if(_m)
        _m->unlock();
}

SharedRecursiveMutex::SharedScopedLock::SharedScopedLock(SharedRecursiveMutex& m) : _m(&m) { _m->lock_shared(); }

SharedRecursiveMutex::SharedScopedLock::SharedScopedLock(SharedScopedLock&& o) noexcept
{
    _m = o._m;
    o._m = nullptr;
}

SharedRecursiveMutex::SharedScopedLock::~SharedScopedLock()
{
    if(_m)
        _m->unlock_shared();
}
