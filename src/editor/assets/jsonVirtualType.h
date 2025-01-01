//
// Created by eli on 8/20/2022.
//

#ifndef BRANEENGINE_JSONVIRTUALTYPE_H
#define BRANEENGINE_JSONVIRTUALTYPE_H

#include <ecs/virtualType.h>
#include <json/json.h>

class JsonVirtualType
{
  public:
    static Json::Value fromVirtual(uint8_t* data, VirtualType::Type type);

    static void toVirtual(uint8_t* data, const Json::Value& source, VirtualType::Type type);
};

#endif // BRANEENGINE_JSONVIRTUALTYPE_H
