#include "Animation.h"

Animation::StopWatch::StopWatch() : _start(std::chrono::high_resolution_clock::now()) {}
void Animation::StopWatch::reset() { _start = std::chrono::high_resolution_clock::now(); }
float Animation::StopWatch::elapsed() const {
        return std::chrono::duration<float, std::chrono::microseconds::period>(std::chrono::high_resolution_clock::now() - _start).count();
}
