#pragma once
#include "trackedObject.h"
#include "utility/shared.h"

template<class T>
class TrackedVector : public TrackedObject
{
    friend struct ModActionCtx;

    struct ModActionCtx : public EditorActionContext
    {
        Shared<TrackedVector<T>> vec;
        Shared<T> value;
        size_t index;
        bool isNewChange = true;
    };

    struct ClearActionCtx : public EditorActionContext
    {
        Shared<TrackedVector<T>> vec;
        Option<std::vector<Shared<T>>> _values;
        bool isNewChange = true;
    };

    static EditorActionExecutor _insertExecutor;
    static EditorActionExecutor _eraseExecutor;
    static EditorActionExecutor _clearExecutor;
    static EditorActionExecutor _restoreExecutor;

  protected:
    static_assert(std::is_base_of<TrackedType, T>());
    Mutex<std::vector<Shared<T>>> _values;

    Event<size_t, Shared<T>, EditorActionType> _onInsert;
    Event<size_t, Shared<T>, EditorActionType> _onErase;

  public:
    void initMembers(Option<std::shared_ptr<TrackedType>> parent)
    {
        TrackedObject::initMembers(parent);
        for(auto& entry : *_values.lock())
            entry->initMembers(parent);
    }

    typename Mutex<std::vector<Shared<T>>>::Lock values()
    {
        return _values.lock();
    }

    const typename Mutex<std::vector<Shared<T>>>::ConstLock values() const
    {
        return _values.lock();
    }

    EditorAction insert(size_t index, Shared<T> value)
    {
        auto ctx = std::make_shared<ModActionCtx>();
        ctx->vec = std::static_pointer_cast<TrackedVector<T>>(shared_from_this());
        ctx->value = value;
        ctx->index = index;
        value->initMembers(Some(shared_from_this()));

        return EditorAction(ctx, _insertExecutor, _eraseExecutor);
    }

    EditorAction erase(size_t index)
    {
        auto ctx = std::make_shared<ModActionCtx>();
        ctx->vec = std::static_pointer_cast<TrackedVector<T>>(shared_from_this());
        ctx->index = index;

        return EditorAction(ctx, _eraseExecutor, _insertExecutor);
    }

    EditorAction push_back(Shared<T> value)
    {
        size_t last = size();
        return insert(last, value);
    }

    EditorAction clear()
    {
        auto ctx = std::make_shared<ClearActionCtx>();
        ctx->vec = std::static_pointer_cast<TrackedVector<T>>(shared_from_this());

        return EditorAction(ctx, _clearExecutor, _restoreExecutor);
    }

    size_t size() const
    {
        return _values.lock()->size();
    }
};

template<class T>
EditorActionExecutor TrackedVector<T>::_insertExecutor =
    [](std::shared_ptr<EditorActionContext> ctx, EditorActionType type) {
    auto actx = std::static_pointer_cast<ModActionCtx>(ctx);
    auto v = actx->vec->_values.lock();
    v->insert(v->begin() + actx->index, actx->value);
    actx->vec->_onChange.invoke(type);
    actx->vec->_onInsert.invoke(actx->index, actx->value, type);
    if(auto p = actx->vec->parent())
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
};

template<class T>
EditorActionExecutor TrackedVector<T>::_eraseExecutor =
    [](std::shared_ptr<EditorActionContext> ctx, EditorActionType type) {
    auto actx = std::static_pointer_cast<ModActionCtx>(ctx);
    auto v = actx->vec->_values.lock();
    if(actx->isNewChange)
        actx->value = (*v)[actx->index];
    actx->vec->_onErase.invoke(actx->index, actx->value, type);
    v->erase(v->begin() + actx->index);
    actx->vec->_onChange.invoke(type);
    if(auto p = actx->vec->parent())
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
};

template<class T>
EditorActionExecutor TrackedVector<T>::_restoreExecutor =
    [](std::shared_ptr<EditorActionContext> ctx, EditorActionType type) {
    auto actx = std::static_pointer_cast<ClearActionCtx>(ctx);
    auto v = actx->vec->_values.lock();
    std::swap(*v, actx->_values.value());
    actx->_values = None();
    size_t index = 0;
    for(auto& entry : *v)
        actx->vec->_onInsert.invoke(index++, entry, type);

    actx->vec->_onChange.invoke(type);

    if(auto p = actx->vec->parent())
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
};

template<class T>
EditorActionExecutor TrackedVector<T>::_clearExecutor =
    [](std::shared_ptr<EditorActionContext> ctx, EditorActionType type) {
    auto actx = std::static_pointer_cast<ClearActionCtx>(ctx);
    auto v = actx->vec->_values.lock();
    actx->_values = Some(std::move(*v));
    v->clear();
    size_t index = 0;
    for(auto& entry : *v)
        actx->vec->_onInsert.invoke(index++, entry, type);

    actx->vec->_onChange.invoke(type);

    if(auto p = actx->vec->parent())
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
};

template<class T>
struct JsonSerializer<TrackedVector<T>>
{
    static Result<void, JsonSerializerError> read(const Json::Value& json, TrackedVector<T>& value)
    {
        if(!json.isArray())
            return Err(JsonSerializerError(JsonSerializerError::WrongType,
                                           std::format("Expected array but found {}", json.toStyledString())));
        value.clear();
        for(auto& entry : json)
        {
            Shared<T> data = std::make_shared<T>();
            CHECK_RESULT(JsonParseUtil::read(entry, data));
            value.push_back(data).forward();
        }
        return Ok<void>();
    }

    static Result<void, JsonSerializerError> write(Json::Value& json, const TrackedVector<T>& value)
    {
        json = Json::Value(Json::ValueType::arrayValue);
        for(auto& entry : *value.values())
        {
            Json::Value data;
            CHECK_RESULT(JsonParseUtil::write(data, entry));
            json.append(data);
        }
        return Ok<void>();
    }
};
