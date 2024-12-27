//
// Created by eli on 7/9/2022.
//

#ifndef BRANEENGINE_LOGGING_H
#define BRANEENGINE_LOGGING_H

#include <ctime>
#include <functional>
#include <string>
#include <vector>

namespace Logging
{
    enum class LogLevel
    {
        error = 0,
        warning = 1,
        log = 2,
        verbose = 3
    };

    enum class LogColor
    {
        reset,
        white,
        red,
        yellow
    };

    struct Log
    {
        std::string message;
        LogLevel level;
        time_t time;

        std::string toString() const;
    };

    extern bool printToConsole;

    void init();

    void cleanup();

    void pushLog(std::string message, LogLevel level);

    size_t addListener(std::function<void(const Log&)> callback);

    void removeListener(size_t index);

    void callListeners();

    void setColor(LogColor color);
}; // namespace Logging

#endif // BRANEENGINE_LOGGING_H
