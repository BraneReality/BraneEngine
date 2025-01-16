#pragma once

#include <memory>
#include <vector>
#include "editor/state/versioning.h"
#include "trackedValue.h"

class TrackedObject : public TrackedType
{
    Event<EditorActionType> _onChange;

  public:
    TrackedObject(Option<std::shared_ptr<TrackedType>> parent);

    Event<EditorActionType>::Handle addListener(Event<EditorActionType>::EventCallback f);

    void childChanged(EditorActionType at) override;
};
