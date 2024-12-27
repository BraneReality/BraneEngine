#pragma once

#include "assets/assembly.h"
#include "ecs/nativeComponent.h"

class AssemblyRoot : public NativeComponent<AssemblyRoot>
{
    REGISTER_MEMBERS_2("Assembly Root", id, "id", loaded, "loaded");

  public:
    AssetID id;
    bool loaded;
};