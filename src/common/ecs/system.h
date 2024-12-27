//
// Created by eli on 7/16/2022.
//

#ifndef BRANEENGINE_SYSTEM_H
#define BRANEENGINE_SYSTEM_H

#include <cstdint>
#include <functional>

class EntityManager;

struct SystemContext
{
    uint32_t version = 0;
    uint32_t lastVersion = 0;
};

class System
{
  protected:
    SystemContext _ctx;

    friend class SystemManager;

  public:
    System() = default;

    virtual ~System() = default;

    virtual void run(EntityManager& em) = 0;
};

#endif // BRANEENGINE_SYSTEM_H
