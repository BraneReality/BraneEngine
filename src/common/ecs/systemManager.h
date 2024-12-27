//
// Created by eli on 7/16/2022.
//

#ifndef BRANEENGINE_SYSTEMMANAGER_H
#define BRANEENGINE_SYSTEMMANAGER_H

#include <memory>
#include <string>
#include "system.h"

class SystemManager
{
    struct SystemNode
    {
        bool hasRun = false;
        bool isRunning = false;
        std::vector<SystemNode*> dependencies;
        std::unique_ptr<System> system;

        SystemNode(std::unique_ptr<System> s);

        void run(EntityManager& em, uint32_t& version);
    };

    std::unordered_map<std::string, std::unique_ptr<SystemNode>> _systems;
    std::unordered_map<std::string, SystemContext> _unmanagedSystems;

  public:
    uint32_t globalVersion = 0;

    // TODO: have an actual scheduling system in here
    void runUnmanagedSystem(const std::string& name, const std::function<void(SystemContext* data)>& f);

    void runSystems(EntityManager& em);

    void addSystem(const std::string& name, std::unique_ptr<System> system);

    bool addDependency(const std::string& systemName, const std::string& dependencyName);
};

#endif // BRANEENGINE_SYSTEMMANAGER_H
