#pragma once
#include <chrono>

class DeltaTimer
{
  public:
    DeltaTimer();
    void Tick();
    double GetDeltaTime();
    double GetDeltaTimeSeconds();
    double GetDeltaTimeMiliseconds();

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> _prev;
    double _delta = 0; // delta time in seconds
    bool _started = false;
};
