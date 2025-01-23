#pragma once

#include <memory>
#include <vector>
#include "editor/state/versioning.h"
#include "trackedValue.h"

class TrackedObject : public TrackedType
{
  protected:
    Event<EditorActionType> _onChange;

  public:
    Event<EditorActionType>::Handle addListener(Event<EditorActionType>::EventCallback f);

    void onChildForward(EditorActionType at) override;
    void onChildBack(EditorActionType at) override;
    void onNewChange() override;
};
