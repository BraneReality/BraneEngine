
#include "trackedValue.h"

Option<std::shared_ptr<TrackedType>> TrackedType::parent()
{
    if(auto parent = _parent.lock())
    {
        return Some(parent);
    }
    return None();
}

void TrackedType::initMembers(Option<std::shared_ptr<TrackedType>> parent)
{
    if(parent)
        _parent = parent.value();
}
