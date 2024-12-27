//
// Created by wirewhiz on 4/13/22.
//

#ifndef BRANEENGINE_RUNTIME_H
#define BRANEENGINE_RUNTIME_H

#include <atomic>
#include <string>
#include <unordered_map>

#include "logging.h"
#include "timeline.h"

class Module;

namespace Runtime
{
    void init();

    void cleanup();

    void addModule(const std::string& name, Module* m);

    template<typename T>
    void addModule()
    {
        static_assert(std::is_base_of<Module, T>());
        addModule(T::name(), new T());
    }

    bool hasModule(const std::string& name);

    template<typename T>
    bool hasModule()
    {
        static_assert(std::is_base_of<Module, T>());
        return hasModule(T::name());
    }

    Module* getModule(const std::string& name);

    template<typename T>
    T* getModule()
    {
        static_assert(std::is_base_of<Module, T>());
        return (T*)getModule(T::name());
    }

    Timeline& timeline();

    void log(const std::string& message);

    void warn(const std::string& message);

    void error(const std::string& message);

    void setTickRate(uint32_t tickRate);

    void run();

    void stop();

    float deltaTime();
}; // namespace Runtime

#endif // BRANEENGINE_RUNTIME_H
