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
            callbacks.erase(
                std::remove_if(callbacks.begin(), callbacks.end(), [id](auto& c) { return c.first == id; }));
        }
    };

    std::shared_ptr<Mutex<EventInner>> _inner;


  public:
    Event()
    {
        _inner = std::make_shared<Mutex<EventInner>>();
    }

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

        Handle(EventHandleId id, std::shared_ptr<Mutex<EventInner>> parent) : _id(id), _parent(parent) {};

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
        return Handle(id, _inner);
    }

    void invoke(Args... args)
    {
        auto inner = _inner->lock();
        for(auto& c : inner->callbacks)
        {
            auto f = c.second;
            ThreadPool::enqueue([f, args...]() { f(args...); });
        }
    }
};
