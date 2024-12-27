#pragma once

#include <atomic>
#include <cassert>
#include <mutex>
#include <thread>
#include <unordered_map>

class SharedRecursiveMutex
{
    std::mutex _m;
    std::thread::id _owner;
    size_t _ownerLockCount = 0;
    size_t _sharedOwners = 0;

  public:
    class ScopedLock
    {
        SharedRecursiveMutex* _m;

      public:
        ScopedLock(SharedRecursiveMutex&);

        ScopedLock(const ScopedLock&) = delete;

        ScopedLock(ScopedLock&&) noexcept;

        ~ScopedLock();
    };

    class SharedScopedLock
    {
        SharedRecursiveMutex* _m;

      public:
        SharedScopedLock(SharedRecursiveMutex&);

        SharedScopedLock(const SharedScopedLock&) = delete;

        SharedScopedLock(SharedScopedLock&&) noexcept;

        ~SharedScopedLock();
    };

    SharedRecursiveMutex();

    void lock();

    void unlock();

    void lock_shared();

    void unlock_shared();

    ScopedLock scopedLock();

    SharedScopedLock sharedScopedLock();
};