#include "DeltaTimer.h"

void DeltaTimer::Tick() {
    if (!_started) { // if the timer hasn't started yet, start it
        _prev = std::chrono::high_resolution_clock::now();
        _started = true;
        return;
    }
    auto now = std::chrono::high_resolution_clock::now();
    _delta = std::chrono::duration<float, std::chrono::seconds::period>(now - _prev).count();
    _prev = std::chrono::high_resolution_clock::now();
}

DeltaTimer::DeltaTimer() { _prev = std::chrono::high_resolution_clock::now(); }

double DeltaTimer::GetDeltaTime() { return _delta; }
