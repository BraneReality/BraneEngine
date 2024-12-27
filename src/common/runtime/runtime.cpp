//
// Created by wirewhiz on 4/13/22.
//

#include "runtime.h"
#include <utility/threadPool.h>

namespace Runtime
{
    std::unordered_map<std::string, std::unique_ptr<Module>> _modules;
    bool _initalized = false;
    Timeline _timeline;
    std::atomic_bool _running = true;
    uint32_t _tickRate = 0;
    std::chrono::high_resolution_clock::time_point _lastUpdate;
    float _deltaTime = 1;

    void addModule(const std::string& name, Module* m)
    {
        assert(_initalized);
        _modules.insert({name, std::unique_ptr<Module>(m)});
    }

    Timeline& timeline() { return _timeline; }

    void log(const std::string& message) { Logging::pushLog(message, Logging::LogLevel::log); }

    void warn(const std::string& message) { Logging::pushLog(message, Logging::LogLevel::warning); }

    void error(const std::string& message) { Logging::pushLog(message, Logging::LogLevel::error); }

    void setTickRate(uint32_t tickRate) { _tickRate = tickRate; }

    void run()
    {
        assert(_initalized);
        for(auto& m : _modules)
        {
            m.second->start();
        }

        while(_running)
        {
            _timeline.run();
            ThreadPool::runMainJobs();
            Logging::callListeners();
            auto now = std::chrono::high_resolution_clock::now();
            auto updatePeriod = std::chrono::duration_cast<std::chrono::microseconds>(now - _lastUpdate);
            _deltaTime = (float)updatePeriod.count() / 1000000.0f;
            _lastUpdate = now;

            if(_tickRate != 0 && updatePeriod.count() < 1000000 / _tickRate)
            {
                std::chrono::microseconds minTime(1000000 / _tickRate);
                std::this_thread::sleep_for(minTime - updatePeriod);
            }
        }
    }

    void stop()
    {
        if(!_running)
            return;
        for(auto& m : _modules)
        {
            m.second->stop();
        }

        _running = false;
    }

    float deltaTime() { return _deltaTime; }

    bool hasModule(const std::string& name) { return _modules.count(name); }

    Module* getModule(const std::string& name)
    {
        auto m = _modules.find(name);
        if(m == _modules.end())
            return nullptr;
        return m->second.get();
    }

    void init()
    {
        Logging::init();
        ThreadPool::init(4);
        _lastUpdate = std::chrono::high_resolution_clock::now();
        _initalized = true;
    }

    void cleanup()
    {
        assert(_initalized);
        stop();
        if(!_modules.empty())
            _modules.clear();
        ThreadPool::cleanup();
        Logging::cleanup();
    }
} // namespace Runtime