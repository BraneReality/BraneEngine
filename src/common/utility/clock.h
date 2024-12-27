#pragma once

#include <chrono>

class Timer
{
  public:
    static std::chrono::high_resolution_clock::time_point lastFrame;
    static uint64_t lastFrameDuration;
    static double deltaTime;
    static unsigned int fps;

    static void startTimer();

    static void updateTimer();
};

class Stopwatch
{
    static std::chrono::high_resolution_clock::time_point _start;

  public:
    Stopwatch();

    template<typename TimeType>
    uint64_t time()
    {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<TimeType>(now - _start).count();
    }
};