#pragma once

#include <memory>
#include <utility>
#include "editor/state/versioning.h"
#include "utility/assert.h"
#include "utility/event.h"
#include "utility/jsonSerializer.h"
#include "utility/mutex.h"

class TrackedType : public std::enable_shared_from_this<TrackedType>
{
    std::weak_ptr<TrackedType> _parent;

  public:
    Option<std::shared_ptr<TrackedType>> parent();

    // We can't get shared_ptr references in teh constructor
    virtual void initMembers(Option<std::shared_ptr<TrackedType>> parent);

    /// Called by children to notify parents of a change
    virtual void onChildForward(EditorActionType at) = 0;
    virtual void onChildBack(EditorActionType at) = 0;
    /// Called to notify savable objects that a new change has been created, and to no longer treat a delta as savable
    virtual void onNewChange() = 0;
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
        bool isNewChange = true;
    };

  public:
    TrackedValue(T init) : _value(Mutex<T>(std::move(init))) {};

    const typename Mutex<T>::ConstLock value() const
    {
        return _value.lock();
    }

    EditorAction set(T value)
    {
        auto ctx = std::make_shared<ActionCtx>();
        ctx->trackedValue = std::static_pointer_cast<TrackedValue<T>>(shared_from_this());
        ctx->before = *_value.lock();
        ctx->after = value;

        return EditorAction(ctx, [](std::shared_ptr<EditorActionContext> ctx, EditorActionType type) {
            auto actx = std::static_pointer_cast<ActionCtx>(ctx);
            auto v = actx->trackedValue->_value.lock();
            *v = actx->after;
            actx->trackedValue->_onChange.invoke(*v, type);
            if(auto p = actx->trackedValue->parent())
            {
                // Once per-change, this has to go in here since we don't know if it's a preview change or if the action
                // gets lost until here in execution
                if(actx->isNewChange && type == EditorActionType::Standard)
                {
                    actx->isNewChange = false;
                    p.value()->onNewChange();
                }
                p.value()->onChildForward(type);
            }
        }, [](std::shared_ptr<EditorActionContext> ctx, EditorActionType type) {
            auto actx = std::static_pointer_cast<ActionCtx>(ctx);
            auto v = actx->trackedValue->_value.lock();
            *v = actx->before;
            actx->trackedValue->_onChange.invoke(*v, type);
            if(auto p = actx->trackedValue->parent())
                p.value()->onChildBack(type);
        });
    }

    Event<T, EditorActionType>::Handle addListener(Event<T, EditorActionType>::EventCallback f)
    {
        return _onChange.addListener(std::move(f));
    }

  protected:
    void onChildForward(EditorActionType at) override {};
    void onChildBack(EditorActionType at) override {};
    void onNewChange() override {};
};

class AssetID;

template<class T>
struct JsonSerializer<TrackedValue<T>>
{
    static Result<void, JsonSerializerError> read(const Json::Value& s, TrackedValue<T>& value)
    {
        T data;
        CHECK_RESULT(JsonSerializer<T>::read(s, data));
        value.set(data).forward();
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& s, const TrackedValue<T>& value)
    {
        CHECK_RESULT(JsonSerializer<T>::write(s, *value.value()));
        return Ok<void>();
    }
};
