//
// Created by wirewhiz on 21/07/22.
//

#ifndef BRANEENGINE_VIRTUALVARIABLEWIDGETS_H
#define BRANEENGINE_VIRTUALVARIABLEWIDGETS_H

#include "common/ecs/component.h"
#include <json/json.h>

enum class UiChangeType
{
    none = 0,
    ongoing = 1,
    finished = 2
};

class VirtualVariableWidgets
{
  public:
    static UiChangeType displayAssetComponentData(Json::Value& component, const Json::Value& assembly);

    static UiChangeType displayVirtualComponentData(VirtualComponentView component);

    static UiChangeType displayVirtualVariable(const char* name,
                                               VirtualType::Type type,
                                               byte* data,
                                               const Json::Value& assembly = Json::nullValue);
};

#endif // BRANEENGINE_VIRTUALVARIABLEWIDGETS_H
