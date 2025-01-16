
#include "trackedObject.h"

TrackedObject::TrackedObject(Option<std::shared_ptr<TrackedType>> parent)
    : TrackedType(
          parent.map<std::weak_ptr<TrackedType>>([](auto t) { return t; }).valueOrDefault(std::weak_ptr<TrackedType>()))
{}

Event<EditorActionType>::Handle TrackedObject::addListener(Event<EditorActionType>::EventCallback f)
{
    return _onChange.addListener(f);
}

void TrackedObject::childChanged(EditorActionType at)
{
    _onChange.invoke(at);
}
