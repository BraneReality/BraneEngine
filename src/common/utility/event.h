#pragma once

#include <functional>
#include <memory>
#include "mutex.h"
#include "utility/threadPool.h"

template<class... Args>
class Event
{
  public:
    using EventCallback = std::function<void(Args...)>;

  private:
    using EventHandleId = size_t;

    struct EventInner
    {
        std::vector<std::pair<EventHandleId, EventCallback>> callbacks;
        EventHandleId counter = 0;

        void removeListener(EventHandleId id)
        {
            auto it = std::remove(callbacks.begin(), callbacks.end(), [id](auto& c) { return c.first == id; });
            auto r = callbacks.end() - it;
            callbacks.erase(it, callbacks.end());
        }
    };

    std::shared_ptr<Mutex<EventInner>> _inner;


  public:
    class Handle
    {
        EventHandleId _id;
        std::shared_ptr<Mutex<EventInner>> _parent;

      public:
        Handle(const Handle&) = delete;

        Handle(Handle&& old)
        {
            _id = old._id;
            old._id = std::numeric_limits<EventHandleId>::max();
        }

        ~Handle()
        {
            if(_id != std::numeric_limits<EventHandleId>::max())
                _parent->lock()->removeListener(_id);
        };
    };

    Handle addListener(EventCallback f)
    {
        auto i = _inner->lock();
        auto id = i->counter++;
        i->callbacks.emplace_back(id, std::move(f));
        return Handle{id, _inner};
    }

    void invoke(Args... args)
    {
        auto inner = _inner.lock();
        for(auto& c : inner.callbacks)
            ThreadPool::enqueue([c, args...]() { c(args...); });
    }
};
