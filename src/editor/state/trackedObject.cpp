
#include "trackedObject.h"

Event<EditorActionType>::Handle TrackedObject::addListener(Event<EditorActionType>::EventCallback f)
{
    return _onChange.addListener(f);
}

void TrackedObject::onChildForward(EditorActionType at)
{
    _onChange.invoke(at);
    if(auto p = parent())
        p.value()->onChildForward(at);
}

void TrackedObject::onChildBack(EditorActionType at)
{
    _onChange.invoke(at);
    if(auto p = parent())
        p.value()->onChildBack(at);
}

void TrackedObject::onNewChange()
{
    if(auto p = parent())
        p.value()->onNewChange();
}
