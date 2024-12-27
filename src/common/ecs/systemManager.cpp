//
// Created by eli on 7/16/2022.
//

#include "systemManager.h"
#include "runtime/runtime.h"

void SystemManager::runSystems(EntityManager& em)
{
    for(auto& system : _systems)
    {
        system.second->hasRun = false;
        system.second->isRunning = false;
    }
    for(auto& system : _systems)
        system.second->run(em, globalVersion);
}

void SystemManager::runUnmanagedSystem(const std::string& name, const std::function<void(SystemContext* data)>& f)
{
    if(!_unmanagedSystems.count(name))
        _unmanagedSystems.insert({name, {0, 0}});
    SystemContext* data = &_unmanagedSystems.at(name);
    data->version = globalVersion++;
    f(data);
    data->lastVersion = data->version;
}

void SystemManager::addSystem(const std::string& name, std::unique_ptr<System> system)
{
    _systems.insert({name, std::make_unique<SystemNode>(std::move(system))});
}

bool SystemManager::addDependency(const std::string& systemName, const std::string& dependencyName)
{
    if(!_systems.count(systemName) || !_systems.count(dependencyName))
        return false;
    _systems.at(systemName)->dependencies.push_back(_systems.at(dependencyName).get());
    return true;
}

SystemManager::SystemNode::SystemNode(std::unique_ptr<System> s) { system = std::move(s); }

void SystemManager::SystemNode::run(EntityManager& em, uint32_t& version)
{
    if(hasRun)
        return;
    if(isRunning)
    {
        Runtime::warn("Circular dependency in systems!");
        return;
    }
    isRunning = true;
    for(auto* dep : dependencies)
        dep->run(em, version);

    system->_ctx.version = version++;
    system->run(em);
    system->_ctx.lastVersion = system->_ctx.version;

    isRunning = false;
    hasRun = true;
}
