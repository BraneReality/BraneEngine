//
// Created by eli on 7/17/2022.
//

#ifndef BRANEENGINE_COMPONENTMANAGER_H
#define BRANEENGINE_COMPONENTMANAGER_H

#include "component.h"

class ComponentManager
{
  public:
    staticIndexVector<std::unique_ptr<ComponentDescription>> _components;
    std::unordered_set<uint16_t> _externalComponents;

  public:
    ~ComponentManager();

    ComponentID createComponent(ComponentAsset* component);

    ComponentID createComponent(const std::vector<VirtualType::Type>& component, const std::string& name);

    ComponentID registerComponent(ComponentDescription* componentDescription);

    const ComponentDescription* getComponentDef(ComponentID id);

    const ComponentDescription* getComponentDef(ComponentAsset* id);

    void eraseComponent(ComponentID id);
};

#endif // BRANEENGINE_COMPONENTMANAGER_H
