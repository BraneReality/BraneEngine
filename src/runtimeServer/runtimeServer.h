#pragma once

#include <vector>
#include <ecs/entity.h>

class RuntimeServer
{
    bool _running;
    EntityManager em;

  public:
    RuntimeServer();

    ~RuntimeServer();

    void createSystems();

    void run();

    static void acceptConnections();
};
