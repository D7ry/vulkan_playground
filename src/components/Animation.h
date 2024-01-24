#include <chrono>

namespace Animation
{
template <typename T>
class StopWatch
{
  public:
    StopWatch() : _start(std::chrono::high_resolution_clock::now()) {}

    void reset() { _start = std::chrono::high_resolution_clock::now(); }

    float elapsed() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float, T>(now - _start).count();
    }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> _start;
};

/**
 * @brief A stop watch that measures time in seconds, with float-point precision.
 */
class StopWatchSeconds : public StopWatch<std::chrono::seconds::period>
{
};
} // namespace Animation