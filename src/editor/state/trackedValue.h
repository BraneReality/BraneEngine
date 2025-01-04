#pragma once

#include <memory>
#include <utility>
#include "editor/state/versioning.h"
#include "utility/assert.h"
#include "utility/event.h"
#include "utility/mutex.h"

class TrackedType : public std::enable_shared_from_this<TrackedType>
{
    std::weak_ptr<TrackedType> _parent;

  protected:
    /// Called by children to notify parents of a change
    virtual void childChanged(EditorActionType at) = 0;

  public:
    TrackedType(std::weak_ptr<TrackedType> parent);

    Option<std::shared_ptr<TrackedType>> parent();
};

template<class T>
class Tracked
{
    static_assert(std::is_base_of<TrackedType, T>(), "Tracked<T> requires that T be derived from TrackedType");
    std::shared_ptr<T> _value;

  public:
    Tracked(T defaultValue, Option<std::shared_ptr<TrackedType>> parent)
    {
        _value = std::make_shared<T>(defaultValue,
                                     parent.map<std::weak_ptr<TrackedType>>([](auto parent) { return parent; }));
    }

    T& operator*()
    {
        return *_value;
    }

    const T& operator*() const
    {
        return *_value;
    }

    T& operator->()
    {
        return *_value;
    }

    const T& operator->() const
    {
        return *_value;
    }
};

template<class T>
class TrackedValue : public TrackedType
{
    Mutex<T> _value;
    Event<T, EditorActionType> _onChange;

    struct ActionCtx : public EditorActionContext
    {
        std::shared_ptr<TrackedValue<T>> trackedValue;
        T before;
        T after;
    };

  public:
    TrackedValue(T init) : _value(std::move(init)) {};

    const Mutex<T>::Lock value() const
    {
        return _value.lock();
    }

    EditorAction set(T value)
    {
        auto ctx = std::make_unique<ActionCtx>();
        ctx->trackedValue = shared_from_this();
        ctx->before = _value;
        ctx->after = value;
        return EditorAction{
            .context = ctx, .forwardAction = [](std::shared_ptr<EditorActionContext> ctx, EditorActionType type) {
            auto actx = std::static_pointer_cast<ActionCtx>(ctx);
            auto v = actx->trackedValue._value.lock();
            *v = actx->after;
            actx->trackedValue._onChange.invoke(*v, type);
            if(auto p = actx->trackedValue.parent())
                p->childChanged(type);
        }, .backAction = [](std::shared_ptr<EditorActionContext> ctx, EditorActionType type) {
            auto actx = std::static_pointer_cast<ActionCtx>(ctx);
            auto v = actx->trackedValue._value.lock();
            *v = actx->before;
            actx->trackedValue._onChange.invoke(*v, type);
            if(auto p = actx->trackedValue.parent())
                p->childChanged(type);
        }};
    }

    Event<T, EditorActionType>::Handle addListener(Event<T, EditorActionType>::EventCallback f) const
    {
        return _onChange.addListener(std::move(f));
    }

    void childChanged(EditorActionType at) override
    {
        BRANE_ASSERT(false, "Cannot call childChanged on TrackedValue!");
    }
};
