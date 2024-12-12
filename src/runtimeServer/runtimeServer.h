#pragma once

#include <ecs/entity.h>
#include <vector>

class RuntimeServer {
    bool _running;
    EntityManager em;

  public:
    RuntimeServer();

    ~RuntimeServer();

    void createSystems();

    void run();

    static void acceptConnections();
};
